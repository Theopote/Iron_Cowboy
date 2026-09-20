#include "Lasso/LassoComponent.h"
#include "AI/HorseBrainComponent.h"
#include "AI/SteppeHerdManager.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Game/SteppeGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/SteppeGameplayTags.h"

ULassoComponent::ULassoComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
}

bool ULassoComponent::BeginAim()
{
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr;
    const auto* Herd=Mode?Mode->HerdManager.Get():nullptr;
    return BeginAimForTarget(Herd?Herd->FocusedHorse.Get():nullptr,Herd && Herd->bTargetIsolated);
}

bool ULassoComponent::BeginAimForTarget(ASteppeWildHorseCharacter* NewTarget, bool bIsolated)
{
    if (State==ELassoState::Attached || State==ELassoState::Subdued || State==ELassoState::Captured) { Feedback=TEXT("Release the attached lasso first"); return false; }
    if (State==ELassoState::Thrown || State==ELassoState::Recovering) { return false; }
    if (!IsValid(NewTarget)) { Feedback=TEXT("Select a target with Q"); return false; }
    Target=NewTarget;
    bTargetIsolated=bIsolated;
    State=ELassoState::Aiming;
    AimSeconds=0.f;
    SwingPhase=0.f;
    SwingStability=0.f;
    HitZone=ELassoHitZone::None;
    Feedback=bTargetIsolated?TEXT("Build the swing, then throw in the stable window"):TEXT("Aim ready - separate the target from the herd");
    return true;
}

void ULassoComponent::CancelAim()
{
    if (State==ELassoState::Aiming)
    {
        State=ELassoState::Stored;
        Target.Reset();
        bTargetIsolated=false;
        AimSeconds=0.f;
        SwingPhase=0.f;
        SwingStability=0.f;
        Feedback=TEXT("Lasso stored");
    }
}

bool ULassoComponent::Throw()
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Controller=Rider?Cast<APlayerController>(Rider->GetController()):nullptr;
    if (!Controller) { return false; }
    FVector Origin; FRotator Rotation;
    Controller->GetPlayerViewPoint(Origin,Rotation);
    const FVector Hand=Rider->GetLassoHandLocation();
    const FVector AimPoint=Origin+Rotation.Vector()*MaximumRange;
    return ThrowFrom(Hand,(AimPoint-Hand).GetSafeNormal());
}

bool ULassoComponent::ThrowFrom(FVector Origin, FVector Direction)
{
    if (State!=ELassoState::Aiming || !Target.IsValid()) { Feedback=TEXT("Hold Right Mouse to aim first"); return false; }
    if (!bTargetIsolated) { Feedback=TEXT("Move the target away from the herd before throwing"); return false; }
    if (FVector::Dist2D(GetOwner()->GetActorLocation(),Target->GetActorLocation())>MaximumRange)
    {
        Feedback=TEXT("Target too far - move within lasso range");
        return false;
    }
    RopeStart=Origin;
    VisualRopeStart=Origin;
    LoopLocation=Origin;
    ThrowDirection=Direction.GetSafeNormal();
    LastThrowStability=FMath::Clamp(SwingStability,0.f,1.f);
    EffectiveCaptureRadius=CaptureRadius*FMath::Lerp(UnstableRadiusMultiplier,1.f,LastThrowStability);
    EffectiveThrowSpeed=ThrowSpeed*FMath::Lerp(UnstableSpeedMultiplier,1.f,LastThrowStability);
    EffectiveMaximumRange=MaximumRange*FMath::Lerp(UnstableRangeMultiplier,1.f,LastThrowStability);
    SwingPlaneNormal=ThrowDirection;
    LoopVelocity=ThrowDirection*EffectiveThrowSpeed+FVector::UpVector*ThrowLift;
    LoopRadius=MinimumLoopRadius;
    LoopAngularPhase=SwingPhase*2.f*PI;
    UpdateLoopAxes();
    TravelDistance=0.f;
    ControlProgress=0.f;
    OnFootSurrenderProgress=0.f;
    bRopeWrapped=false;
    RopeBendPoint=FVector::ZeroVector;
    RopeWrapClearTime=0.f;
    Tension=0.f;
    ShockRiskSeconds=0.f;
    ShockLoad=0.f;
    SeparatingSpeed=0.f;
    AnchorDeceleration=0.f;
    bShockRisk=false;
    bHadAnchorSample=false;
    HitZone=ELassoHitZone::None;
    State=ELassoState::Thrown;
    Feedback=FString::Printf(TEXT("Lasso in flight | stability %.0f%%"),LastThrowStability*100.f);
    return true;
}

void ULassoComponent::StartRecovery(const TCHAR* Message)
{
    if (auto* Horse=Target.Get())
    {
        Horse->Brain->SetLassoed(false);
        CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearExternalAcceleration();
    }
    Target.Reset();
    bTargetIsolated=false;
    bBracing=false;
    Tension=0.f;
    ShockLoad=0.f;
    SeparatingSpeed=0.f;
    AnchorDeceleration=0.f;
    bShockRisk=false;
    ShockRiskSeconds=0.f;
    bHadAnchorSample=false;
    ControlProgress=0.f;
    OnFootSurrenderProgress=0.f;
    bRopeWrapped=false;
    RopeBendPoint=FVector::ZeroVector;
    RopeWrapClearTime=0.f;
    HitZone=ELassoHitZone::None;
    State=ELassoState::Recovering;
    RecoveryRemaining=FMath::Max(.1f,RecoverySeconds);
    Feedback=Message;
}

void ULassoComponent::Release()
{
    if (State==ELassoState::Attached || State==ELassoState::Subdued || State==ELassoState::Captured)
    {
        StartRecovery(State==ELassoState::Captured?TEXT("Capture secured - lasso recovering"):TEXT("Lasso released"));
    }
}

bool ULassoComponent::Capture()
{
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr;
    return CaptureWithHerd(Mode?Mode->HerdManager.Get():nullptr);
}

bool ULassoComponent::CaptureWithHerd(ASteppeHerdManager* Herd)
{
    auto* Horse=Target.Get();
    if (State!=ELassoState::Subdued || !Horse || !Herd) { Feedback=TEXT("Subdue the target before capture"); return false; }
    if (!Herd->RegisterCapturedHorse(Horse)) { Feedback=TEXT("Capture registration failed"); return false; }
    Horse->Brain->SetCaptured(true);
    CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearExternalAcceleration();
    State=ELassoState::Captured;
    bBracing=false;
    Tension=0.f;
    ControlProgress=1.f;
    Feedback=FString::Printf(TEXT("CAPTURED | herd secured %d | LMB stow"),Herd->CapturedCount);
    return true;
}

bool ULassoComponent::CompleteOnFootSurrender(ASteppeHerdManager* Herd)
{
    auto* Horse=Target.Get();
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    if (State!=ELassoState::Attached || OnFootSurrenderProgress<1.f || !Horse || !Rider || !Herd
        || (Rider->Riding && Rider->Riding->IsMounted()) || !Herd->AcceptRopeSurrender(Horse,Rider))
    {
        return false;
    }
    State=ELassoState::Captured;
    CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearExternalAcceleration();
    bBracing=false;
    Tension=0.f;
    ControlProgress=1.f;
    Feedback=TEXT("HORSE SURRENDERED | lead it to CAMP / PEN");
    return true;
}

FGameplayTag ULassoComponent::GetStateTag() const
{
    switch (State)
    {
    case ELassoState::Aiming: return SteppeTags::Lasso_State_Aiming;
    case ELassoState::Thrown: return SteppeTags::Lasso_State_Thrown;
    case ELassoState::Attached: return SteppeTags::Lasso_State_Attached;
    case ELassoState::Recovering: return SteppeTags::Lasso_State_Recovering;
    case ELassoState::Subdued: return SteppeTags::Lasso_State_Subdued;
    case ELassoState::Captured: return SteppeTags::Lasso_State_Captured;
    default: return SteppeTags::Lasso_State_Stored;
    }
}

float ULassoComponent::GetEffectiveSubdueSeconds(const ASteppeWildHorseCharacter* Horse) const
{
    const float ZoneMultiplier=HitZone==ELassoHitZone::Neck?.85f:(HitZone==ELassoHitZone::Torso?1.25f:(HitZone==ELassoHitZone::Head?1.05f:1.f));
    return FMath::Max(.1f,SubdueSeconds*(Horse?Horse->SubdueResistance:1.f)*ZoneMultiplier);
}

ELassoHitZone ULassoComponent::ClassifyHitZone(const ASteppeWildHorseCharacter* Horse, FVector HitLocation) const
{
    return Horse && Horse->LassoTarget?Horse->LassoTarget->ClassifyLocation(HitLocation):ELassoHitZone::None;
}

float ULassoComponent::GetHitZoneTensionMultiplier() const
{
    return HitZone==ELassoHitZone::Head?1.15f:(HitZone==ELassoHitZone::Torso?.9f:1.f);
}

float ULassoComponent::CalculateShockLoad(float InSeparatingSpeed, float InAnchorDeceleration, float TensionValue) const
{
    const float SpeedRatio=FMath::Max(0.f,InSeparatingSpeed)/FMath::Max(1.f,ShockSpeedThreshold);
    const float DecelerationRatio=FMath::Max(0.f,InAnchorDeceleration)/FMath::Max(1.f,ShockDecelerationThreshold);
    return FMath::Clamp(SpeedRatio*DecelerationRatio*FMath::Max(.35f,TensionValue),0.f,2.f);
}

void ULassoComponent::UpdateSwing(float Dt)
{
    if (const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr)
    {
        if (const auto* Herd=Mode->HerdManager.Get(); Herd && Herd->FocusedHorse==Target.Get())
        {
            bTargetIsolated=Herd->bTargetIsolated;
        }
    }
    AimSeconds+=FMath::Max(0.f,Dt);
    SwingPhase=FMath::Fmod(AimSeconds/FMath::Max(.2f,SwingPeriod),1.f);
    const float Ready=FMath::Clamp(AimSeconds/FMath::Max(.1f,ReadySeconds),0.f,1.f);
    const float Timing=1.f-FMath::Abs(SwingPhase-.5f)*2.f;
    SwingStability=Ready*FMath::Lerp(.25f,1.f,FMath::Clamp(Timing,0.f,1.f));
    Feedback=!bTargetIsolated?TEXT("Aim ready - separate the target from the herd"):
        (SwingStability>=.8f?TEXT("Stable window - throw now"):TEXT("Swinging - wait for the loop to open"));
}

void ULassoComponent::UpdateLoopAxes()
{
    const FVector Normal=SwingPlaneNormal.GetSafeNormal(KINDA_SMALL_NUMBER,FVector::ForwardVector);
    FVector BaseX=FVector::CrossProduct(FVector::UpVector,Normal).GetSafeNormal();
    if (BaseX.IsNearlyZero()) { BaseX=FVector::RightVector; }
    const FVector BaseY=FVector::CrossProduct(Normal,BaseX).GetSafeNormal();
    LoopAxisX=BaseX.RotateAngleAxis(FMath::RadiansToDegrees(LoopAngularPhase),Normal);
    LoopAxisY=BaseY.RotateAngleAxis(FMath::RadiansToDegrees(LoopAngularPhase),Normal);
}

bool ULassoComponent::FindPhysicalLoopHit(const FVector& PreviousCenter, const FVector& NextCenter,
    ASteppeWildHorseCharacter*& OutHorse, FVector& OutHitLocation, ELassoHitZone& OutZone) const
{
    OutHorse=nullptr;
    OutHitLocation=FVector::ZeroVector;
    OutZone=ELassoHitZone::None;
    UWorld* World=GetWorld();
    if (!World) { return false; }
    float BestAlong=BIG_NUMBER;
    for (TActorIterator<ASteppeWildHorseCharacter> It(World); It; ++It)
    {
        ASteppeWildHorseCharacter* Horse=*It;
        if (!IsValid(Horse) || !Horse->Brain || Horse->Brain->bCaptured || !Horse->LassoTarget) { continue; }
        float Along=BIG_NUMBER;
        FVector Location=FVector::ZeroVector;
        ELassoHitZone Zone=ELassoHitZone::None;
        if (Horse->LassoTarget->FindLoopIntersection(PreviousCenter,NextCenter,SwingPlaneNormal,
            LoopRadius,LoopPlaneThickness,Along,Location,Zone) && Along<BestAlong)
        {
            BestAlong=Along;
            OutHorse=Horse;
            OutHitLocation=Location;
            OutZone=Zone;
        }
    }
    return OutHorse!=nullptr;
}

void ULassoComponent::AttachHorse(ASteppeWildHorseCharacter* Horse, const FVector& HitLocation, ELassoHitZone Zone)
{
    if (!Horse) { return; }
    Target=Horse;
    bTargetIsolated=true;
    State=ELassoState::Attached;
    HitZone=Zone;
    LoopLocation=Horse->GetActorLocation()+FVector(0,0,90);
    RopeLength=FMath::Max(200.f,FVector::Dist(RopeStart,LoopLocation)-120.f);
    Tension=120.f/FMath::Max(10.f,TensionRange)*GetHitZoneTensionMultiplier();
    ShockLoad=0.f;
    ShockRiskSeconds=0.f;
    bShockRisk=false;
    bHadAnchorSample=false;
    OnFootSurrenderProgress=0.f;
    bRopeWrapped=false;
    RopeBendPoint=FVector::ZeroVector;
    RopeWrapClearTime=0.f;
    Horse->Brain->SetLassoed(true);
    Feedback=FString::Printf(TEXT("%s PHYSICAL LOOP - Left Mouse to release"),*UEnum::GetDisplayValueAsText(HitZone).ToString().ToUpper());
}

void ULassoComponent::UpdateRopeObstacle(float Dt)
{
    auto* Horse=Target.Get();
    UWorld* World=GetWorld();
    if (!World || !Horse) { bRopeWrapped=false; RopeWrapClearTime=0.f; return; }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeRopeObstacle),false,GetOwner());
    Params.AddIgnoredActor(Horse);
    if (const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
    {
        if (Rider->Riding && Rider->Riding->GetHorse()) { Params.AddIgnoredActor(Rider->Riding->GetHorse()); }
    }
    FCollisionObjectQueryParams Obstacles;
    Obstacles.AddObjectTypesToQuery(ECC_WorldStatic);
    Obstacles.AddObjectTypesToQuery(ECC_WorldDynamic);
    FHitResult Hit;
    if (World->LineTraceSingleByObjectType(Hit,RopeStart,LoopLocation,Obstacles,Params))
    {
        bRopeWrapped=true;
        RopeBendPoint=Hit.ImpactPoint+Hit.ImpactNormal*12.f;
        RopeWrapClearTime=0.f;
    }
    else if (bRopeWrapped)
    {
        RopeWrapClearTime+=Dt;
        if (RopeWrapClearTime>=RopeWrapClearSeconds)
        {
            bRopeWrapped=false;
            RopeBendPoint=FVector::ZeroVector;
            RopeWrapClearTime=0.f;
        }
    }
}

void ULassoComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (State==ELassoState::Aiming)
    {
        UpdateSwing(Dt);
        return;
    }
    if (State==ELassoState::Thrown || State==ELassoState::Attached || State==ELassoState::Subdued || State==ELassoState::Captured)
    {
        const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
        // Gameplay tension uses a stable holder anchor; animated hands only move the drawn rope.
        RopeStart=GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*55.f
            +GetOwner()->GetActorRightVector()*40.f+FVector(0,0,115.f);
        VisualRopeStart=Rider?Rider->GetLassoHandLocation():RopeStart;
    }
    if (State==ELassoState::Recovering)
    {
        RecoveryRemaining-=Dt;
        if (RecoveryRemaining<=0.f) { State=ELassoState::Stored; Feedback=TEXT("Lasso ready"); }
        return;
    }
    if (State==ELassoState::Attached || State==ELassoState::Subdued || State==ELassoState::Captured)
    {
        if (auto* Horse=Target.Get())
        {
            LoopLocation=Horse->GetActorLocation()+FVector(0,0,90);
            UpdateRopeObstacle(Dt);
            const FVector ConstraintAnchor=bRopeWrapped?RopeBendPoint:RopeStart;
            const float Distance=bRopeWrapped
                ?FVector::Dist(RopeStart,RopeBendPoint)+FVector::Dist(RopeBendPoint,LoopLocation)
                :FVector::Dist(RopeStart,LoopLocation);
            if (State==ELassoState::Captured)
            {
                Horse->Brain->SetCaptured(true);
                CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearExternalAcceleration();
                Feedback=TEXT("CAPTURED | LMB stow lasso");
                return;
            }
            if (State==ELassoState::Subdued)
            {
                Horse->Brain->SetLassoConstraint(ConstraintAnchor,1.f,true,1.f,bRopeWrapped);
                CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearExternalAcceleration();
                Feedback=FString::Printf(TEXT("%s LOOP | SUBDUED - press C"),*UEnum::GetDisplayValueAsText(HitZone).ToString().ToUpper());
                return;
            }
            const FVector RopeDirection=(LoopLocation-ConstraintAnchor).GetSafeNormal();
            FVector AnchorVelocity=bRopeWrapped?FVector::ZeroVector:GetOwner()->GetVelocity();
            if (!bRopeWrapped)
            {
                if (const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
                {
                    if (Rider->Riding && Rider->Riding->GetHorse()) { AnchorVelocity=Rider->Riding->GetHorse()->GetVelocity(); }
                }
            }
            const float AnchorSpeed=AnchorVelocity.Size2D();
            SeparatingSpeed=FVector::DotProduct(Horse->GetVelocity()-AnchorVelocity,RopeDirection);
            AnchorDeceleration=bHadAnchorSample && Dt>SMALL_NUMBER?FMath::Max(0.f,(PreviousAnchorSpeed-AnchorSpeed)/Dt):0.f;
            PreviousAnchorSpeed=AnchorSpeed;
            bHadAnchorSample=true;
            Tension=FMath::Clamp(((Distance-RopeLength)/FMath::Max(10.f,TensionRange)+FMath::Max(0.f,SeparatingSpeed)/1200.f)*GetHitZoneTensionMultiplier()
                +(bRopeWrapped?ObstacleWrapTensionBonus:0.f),0.f,1.5f);
            CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->SetExternalAcceleration(
                (ConstraintAnchor-Horse->GetActorLocation()).GetSafeNormal2D()*Tension*HorsePullAcceleration);
            const float NewShock=CalculateShockLoad(SeparatingSpeed,AnchorDeceleration,Tension);
            ShockLoad=FMath::Max(NewShock,FMath::Max(0.f,ShockLoad-ShockDecayPerSecond*Dt));
            auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
            const bool bMounted=Rider && Rider->Riding && Rider->Riding->IsMounted();
            bShockRisk=bMounted && !bRopeWrapped && ShockLoad>=ShockBreakThreshold;
            ShockRiskSeconds=bShockRisk?ShockRiskSeconds+Dt:FMath::Max(0.f,ShockRiskSeconds-Dt*3.f);
            if (!bMounted) { ShockRiskSeconds=0.f; ShockLoad=FMath::Max(0.f,ShockLoad-ShockDecayPerSecond*3.f*Dt); }
            Horse->Brain->SetLassoConstraint(ConstraintAnchor,Tension,bBracing,ControlProgress,bRopeWrapped);
            const bool bSpeedFullyRatchet=Horse->Brain->LassoSpeedLimitScale<=Horse->Brain->MinimumLassoSpeedLimitScale+.02f;
            const float EffectiveUsefulTensionMax=(bRopeWrapped || bSpeedFullyRatchet)?1.5f:UsefulTensionMax;
            const bool bUseful=bBracing && Tension>=UsefulTensionMin && Tension<=EffectiveUsefulTensionMax;
            ControlProgress=FMath::Clamp(ControlProgress+(bUseful?Dt:-Dt*.6f)/GetEffectiveSubdueSeconds(Horse),0.f,1.f);
            const bool bCloseOnFoot=!bMounted && Rider && bBracing && FVector::Dist2D(Rider->GetActorLocation(),Horse->GetActorLocation())<=OnFootSurrenderDistance
                && FMath::Abs(SeparatingSpeed)<=OnFootSurrenderMaxRelativeSpeed;
            OnFootSurrenderProgress=FMath::Clamp(OnFootSurrenderProgress+(bCloseOnFoot?Dt:-Dt*.75f)/FMath::Max(.1f,OnFootSurrenderSeconds),0.f,1.f);
            if (Distance>MaximumRange*EmergencyBreakRangeMultiplier) { StartRecovery(TEXT("Rope severed at extreme distance - recovering")); }
            else if (ShockRiskSeconds>=BreakHoldSeconds) { StartRecovery(TEXT("Sudden stop snapped the rope - recovering")); }
            else if (OnFootSurrenderProgress>=1.f)
            {
                auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr;
                CompleteOnFootSurrender(Mode?Mode->HerdManager.Get():nullptr);
            }
            else if (ControlProgress>=1.f)
            {
                State=ELassoState::Subdued;
                Feedback=FString::Printf(TEXT("%s LOOP | SUBDUED - press C"),*UEnum::GetDisplayValueAsText(HitZone).ToString().ToUpper());
            }
            else
            {
                const TCHAR* FightHint=bUseful?TEXT("tension steady"):bBracing?TEXT("adjust into green tension"):TEXT("hold Space to brace");
                Feedback=FString::Printf(TEXT("%s LOOP | %s"),*UEnum::GetDisplayValueAsText(HitZone).ToString().ToUpper(),FightHint);
            }
        }
        else { StartRecovery(TEXT("Target lost")); }
        return;
    }
    if (State!=ELassoState::Thrown) { return; }
    if (!Target.IsValid()) { StartRecovery(TEXT("Target lost")); return; }

    const FVector Previous=LoopLocation;
    const float RemainingRange=FMath::Max(0.f,EffectiveMaximumRange-TravelDistance);
    const float RequestedStep=LoopVelocity.Size()*Dt;
    const float StepScale=RequestedStep>SMALL_NUMBER?FMath::Min(1.f,RemainingRange/RequestedStep):0.f;
    const float StepTime=Dt*StepScale;
    const FVector Gravity(0,0,-LoopGravity);
    const FVector Next=Previous+LoopVelocity*StepTime+Gravity*(.5f*StepTime*StepTime);
    LoopVelocity+=Gravity*StepTime;
    SwingPlaneNormal=LoopVelocity.GetSafeNormal(KINDA_SMALL_NUMBER,SwingPlaneNormal);
    LoopAngularPhase=FMath::Fmod(LoopAngularPhase+FMath::DegreesToRadians(FlightSpinDegreesPerSecond)*StepTime,2.f*PI);
    LoopRadius=FMath::Lerp(MinimumLoopRadius,EffectiveCaptureRadius,
        FMath::Clamp((TravelDistance+FVector::Dist(Previous,Next))/FMath::Max(1.f,LoopOpeningDistance),0.f,1.f));
    UpdateLoopAxes();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeLasso),false,GetOwner());
    if (const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
    {
        if (Rider->Riding && Rider->Riding->GetHorse()) { Params.AddIgnoredActor(Rider->Riding->GetHorse()); }
    }
    FCollisionObjectQueryParams Obstacles;
    Obstacles.AddObjectTypesToQuery(ECC_WorldStatic);
    Obstacles.AddObjectTypesToQuery(ECC_WorldDynamic);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByObjectType(Hit,Previous,Next,Obstacles,Params))
    {
        LoopLocation=Hit.ImpactPoint;
        StartRecovery(TEXT("Lasso blocked - recovering"));
        return;
    }
    ASteppeWildHorseCharacter* CaughtHorse=nullptr;
    FVector PhysicalHitLocation=FVector::ZeroVector;
    ELassoHitZone PhysicalHitZone=ELassoHitZone::None;
    if (FindPhysicalLoopHit(Previous,Next,CaughtHorse,PhysicalHitLocation,PhysicalHitZone))
    {
        AttachHorse(CaughtHorse,PhysicalHitLocation,PhysicalHitZone);
        return;
    }
    LoopLocation=Next;
    TravelDistance+=FVector::Dist(Previous,Next);
    if (TravelDistance>=EffectiveMaximumRange-KINDA_SMALL_NUMBER) { StartRecovery(TEXT("Missed - recovering")); }
}

void ULassoComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Horse=Target.Get())
    {
        Horse->Brain->SetCaptured(false);
        Horse->Brain->SetLassoed(false);
        CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearExternalAcceleration();
    }
    Target.Reset();
    Super::EndPlay(Reason);
}
