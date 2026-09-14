#include "Lasso/LassoComponent.h"
#include "AI/HorseBrainComponent.h"
#include "AI/SteppeHerdManager.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Game/SteppeGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
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
    return ThrowFrom(Origin,Rotation.Vector());
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
    LoopLocation=Origin;
    ThrowDirection=Direction.GetSafeNormal();
    LastThrowStability=FMath::Clamp(SwingStability,0.f,1.f);
    EffectiveCaptureRadius=CaptureRadius*FMath::Lerp(UnstableRadiusMultiplier,1.f,LastThrowStability);
    EffectiveThrowSpeed=ThrowSpeed*FMath::Lerp(UnstableSpeedMultiplier,1.f,LastThrowStability);
    EffectiveMaximumRange=MaximumRange*FMath::Lerp(UnstableRangeMultiplier,1.f,LastThrowStability);
    TravelDistance=0.f;
    ControlProgress=0.f;
    Tension=0.f;
    OverTensionSeconds=0.f;
    HitZone=ELassoHitZone::None;
    State=ELassoState::Thrown;
    Feedback=FString::Printf(TEXT("Lasso in flight | stability %.0f%%"),LastThrowStability*100.f);
    return true;
}

void ULassoComponent::StartRecovery(const TCHAR* Message)
{
    if (auto* Horse=Target.Get()) { Horse->Brain->SetLassoed(false); }
    Target.Reset();
    bTargetIsolated=false;
    bBracing=false;
    Tension=0.f;
    ControlProgress=0.f;
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
    State=ELassoState::Captured;
    bBracing=false;
    Tension=0.f;
    ControlProgress=1.f;
    Feedback=FString::Printf(TEXT("CAPTURED | herd secured %d | LMB stow"),Herd->CapturedCount);
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
    if (!Horse) { return ELassoHitZone::None; }
    const float LocalHeight=Horse->GetActorTransform().InverseTransformPosition(HitLocation).Z;
    return LocalHeight>85.f?ELassoHitZone::Head:(LocalHeight>=35.f?ELassoHitZone::Neck:ELassoHitZone::Torso);
}

float ULassoComponent::GetHitZoneTensionMultiplier() const
{
    return HitZone==ELassoHitZone::Head?1.15f:(HitZone==ELassoHitZone::Torso?.9f:1.f);
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
        RopeStart=GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*55.f+GetOwner()->GetActorRightVector()*40.f+FVector(0,0,115);
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
            const float Distance=FVector::Dist(RopeStart,LoopLocation);
            if (State==ELassoState::Captured)
            {
                Horse->Brain->SetCaptured(true);
                Feedback=TEXT("CAPTURED | LMB stow lasso");
                return;
            }
            if (State==ELassoState::Subdued)
            {
                Horse->Brain->SetLassoConstraint(RopeStart,1.f,true);
                Feedback=FString::Printf(TEXT("%s LOOP | SUBDUED - press C"),*UEnum::GetDisplayValueAsText(HitZone).ToString().ToUpper());
                return;
            }
            const FVector RopeDirection=(LoopLocation-RopeStart).GetSafeNormal();
            FVector AnchorVelocity=GetOwner()->GetVelocity();
            if (const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
            {
                if (Rider->Riding && Rider->Riding->GetHorse()) { AnchorVelocity=Rider->Riding->GetHorse()->GetVelocity(); }
            }
            const float SeparatingSpeed=FVector::DotProduct(Horse->GetVelocity()-AnchorVelocity,RopeDirection);
            Tension=FMath::Clamp(((Distance-RopeLength)/FMath::Max(10.f,TensionRange)+FMath::Max(0.f,SeparatingSpeed)/1200.f)*GetHitZoneTensionMultiplier(),0.f,1.5f);
            Horse->Brain->SetLassoConstraint(RopeStart,Tension,bBracing);
            const bool bUseful=bBracing && Tension>=UsefulTensionMin && Tension<=UsefulTensionMax;
            ControlProgress=FMath::Clamp(ControlProgress+(bUseful?Dt:-Dt*.6f)/GetEffectiveSubdueSeconds(Horse),0.f,1.f);
            OverTensionSeconds=Tension>1.f?OverTensionSeconds+Dt:FMath::Max(0.f,OverTensionSeconds-Dt*2.f);
            if (Distance>MaximumRange*1.1f || OverTensionSeconds>=BreakHoldSeconds) { StartRecovery(TEXT("Rope broke - recovering")); }
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

    const float Step=FMath::Min(EffectiveThrowSpeed*Dt,EffectiveMaximumRange-TravelDistance);
    const FVector Previous=LoopLocation;
    const FVector Next=Previous+ThrowDirection*FMath::Max(0.f,Step);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeLasso),false,GetOwner());
    if (const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
    {
        if (Rider->Riding && Rider->Riding->GetHorse()) { Params.AddIgnoredActor(Rider->Riding->GetHorse()); }
    }
    FCollisionObjectQueryParams Pawns;
    Pawns.AddObjectTypesToQuery(ECC_Pawn);
    Pawns.AddObjectTypesToQuery(ECC_WorldStatic);
    Pawns.AddObjectTypesToQuery(ECC_WorldDynamic);
    FHitResult Hit;
    if (GetWorld()->SweepSingleByObjectType(Hit,Previous,Next,FQuat::Identity,Pawns,FCollisionShape::MakeSphere(EffectiveCaptureRadius),Params))
    {
        LoopLocation=Hit.ImpactPoint;
        if (Hit.GetActor()==Target.Get())
        {
            State=ELassoState::Attached;
            HitZone=ClassifyHitZone(Target.Get(),Hit.ImpactPoint);
            RopeLength=FMath::Max(200.f,FVector::Dist(RopeStart,LoopLocation)-120.f);
            Tension=120.f/FMath::Max(10.f,TensionRange)*GetHitZoneTensionMultiplier();
            Target->Brain->SetLassoed(true);
            Feedback=FString::Printf(TEXT("%s LOOP - Left Mouse to release"),*UEnum::GetDisplayValueAsText(HitZone).ToString().ToUpper());
        }
        else { StartRecovery(TEXT("Lasso blocked - recovering")); }
        return;
    }
    LoopLocation=Next;
    TravelDistance+=Step;
    if (TravelDistance>=EffectiveMaximumRange-KINDA_SMALL_NUMBER) { StartRecovery(TEXT("Missed - recovering")); }
}

void ULassoComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Horse=Target.Get()) { Horse->Brain->SetCaptured(false); Horse->Brain->SetLassoed(false); }
    Target.Reset();
    Super::EndPlay(Reason);
}
