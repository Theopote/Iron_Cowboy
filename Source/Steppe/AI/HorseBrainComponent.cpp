#include "AI/HorseBrainComponent.h"
#include "AI/WildHorseConfig.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "Components/CapsuleComponent.h"
#include "Core/SteppeGameplayTags.h"
#include "Engine/World.h"
#include "Steppe.h"

UHorseBrainComponent::UHorseBrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}
const UWildHorseConfig& UHorseBrainComponent::GetConfig() const
{
    return *(Config ? Config.Get() : GetDefault<UWildHorseConfig>());
}
void UHorseBrainComponent::BeginPlay()
{
    Super::BeginPlay();
    Home = GetOwner()->GetActorLocation();
    const int32 Seed=IdentitySeed!=0?IdentitySeed:GetConfig().RandomSeed;
    Random.Initialize(Seed);
    SetComponentTickInterval(FMath::Max(.02f, GetConfig().DecisionInterval*IndividualReactionScale));
    if (auto* Horse = Cast<ASteppeHorseCharacter>(GetOwner()))
    {
        Horse->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
    }
    ChooseRoamGoal();
}
void UHorseBrainComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Horse = Cast<ASteppeHorseCharacter>(GetOwner()))
    {
        Horse->GetCharacterMovement()->RemoveTickPrerequisiteComponent(this);
        if (auto* Movement = Cast<UHorseMovementComponent>(Horse->GetCharacterMovement())) { Movement->ClearIntent(); }
    }
    Super::EndPlay(Reason);
}
void UHorseBrainComponent::SetThreatTarget(AActor* Target)
{
    ThreatTarget = Target == GetOwner() ? nullptr : Target;
    bThreatVisible = false;
    ClosingSpeed = 0.f;
    ApproachSpeed = 0.f;
    // Keep the last observed position briefly: losing sight must not instantly cancel a chase.
}
void UHorseBrainComponent::SetHerdIdentity(int32 MemberIndex, int32 HerdSeed)
{
    HerdMemberIndex=MemberIndex;
    IdentitySeed=HashCombineFast(static_cast<uint32>(HerdSeed),static_cast<uint32>(MemberIndex+1)*2654435761u);
    FRandomStream Personality(IdentitySeed);
    const float Variation=FMath::Clamp(GetConfig().ReactionTimeVariation,0.f,.75f);
    IndividualReactionScale=Personality.FRandRange(1.f-Variation,1.f+Variation);
    IndividualSteeringBias=Personality.FRandRange(-GetConfig().IndividualSteeringDegrees,GetConfig().IndividualSteeringDegrees);
    IndividualPauseScale=Personality.FRandRange(.65f,1.35f);
}
void UHorseBrainComponent::ReceiveHerdAlarm(float Strength, float Duration)
{
    HerdAlarmStrength=FMath::Clamp(Strength,0.f,1.f);
    HerdAlarmSeconds=FMath::Max(HerdAlarmSeconds,FMath::Max(0.f,Duration));
}
void UHorseBrainComponent::SetLassoed(bool bNewLassoed)
{
    if (bNewLassoed && !bLassoed) { LassoSpeedLimitScale=FMath::Clamp(InitialLassoSpeedLimitScale,MinimumLassoSpeedLimitScale,1.f); }
    bLassoed=bNewLassoed;
    if (bLassoed) { ChangeState(EWildHorseState::Lassoed); }
    else if (State==EWildHorseState::Lassoed) { ChangeState(EWildHorseState::Recovering); }
}
void UHorseBrainComponent::SetLassoConstraint(FVector Anchor, float Tension, bool bBraced, float ControlProgress, bool bObstacleWrapped)
{
    LassoAnchor=Anchor;
    LassoTension=FMath::Clamp(Tension,0.f,1.5f);
    bLassoBraced=bBraced;
    LassoControlProgress=FMath::Clamp(ControlProgress,0.f,1.f);
    bLassoObstacleWrapped=bObstacleWrapped;
    const float Tightening=FMath::Max(LassoControlProgress,FMath::Clamp((LassoTension-.15f)/.7f,0.f,1.f));
    float NewLimit=FMath::Lerp(InitialLassoSpeedLimitScale,MinimumLassoSpeedLimitScale,Tightening);
    if (bLassoObstacleWrapped) { NewLimit*=.65f; }
    if (bLassoBraced || bLassoObstacleWrapped) { LassoSpeedLimitScale=FMath::Min(LassoSpeedLimitScale,FMath::Max(.08f,NewLimit)); }
}
void UHorseBrainComponent::SetCaptured(bool bNewCaptured)
{
    bCaptured=bNewCaptured;
    if (bCaptured)
    {
        bLassoed=true;
        ChangeState(EWildHorseState::Captured);
    }
    else if (State==EWildHorseState::Captured) { ChangeState(EWildHorseState::Recovering); }
}
void UHorseBrainComponent::RequestCapturedRetreat(FVector Direction, float Speed, float Duration)
{
    if (!bCaptured) { return; }
    CapturedRetreatDirection=Direction.GetSafeNormal2D();
    CapturedRetreatSpeed=FMath::Max(0.f,Speed);
    CapturedRetreatSeconds=FMath::Max(CapturedRetreatSeconds,FMath::Max(0.f,Duration));
}
void UHorseBrainComponent::SetLeadTarget(AActor* Target)
{
    LeadTarget=Target==GetOwner()?nullptr:Target;
    bLeading=LeadTarget.IsValid();
    LeadDistance=0.f;
}
void UHorseBrainComponent::SetHerdGuidance(FVector Center, FVector Velocity, FVector EscapeDirection, FVector Separation, int32 NeighborCount)
{
    HerdCenter=Center;
    HerdVelocity=Velocity;
    HerdEscapeDirection=EscapeDirection.GetSafeNormal2D();
    HerdSeparation=Separation;
    HerdNeighborCount=FMath::Max(0,NeighborCount);
}
void UHorseBrainComponent::Sense(float Dt, const ASteppeHorseCharacter& Horse)
{
    const auto& C = GetConfig();
    bThreatVisible = false;
    ThreatDistance = 0.f;
    ClosingSpeed = 0.f;
    ApproachSpeed = 0.f;
    DynamicAvoidance = FVector::ZeroVector;
    if (AActor* Target = ThreatTarget.Get())
    {
        const FVector ToHorse = (Horse.GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
        ThreatDistance = FVector::Dist2D(Horse.GetActorLocation(), Target->GetActorLocation());
        // An attached rider's own movement component is disabled; use the mount's velocity.
        const AActor* MotionSource = Target->GetAttachParentActor() ? Target->GetAttachParentActor() : Target;
        ApproachSpeed = FVector::DotProduct(MotionSource->GetVelocity(), ToHorse);
        ClosingSpeed = FMath::Max(0.f, FVector::DotProduct(MotionSource->GetVelocity() - Horse.GetVelocity(), ToHorse));
        const FVector FromDynamic=Horse.GetActorLocation()-MotionSource->GetActorLocation();
        const float DynamicDistance=FromDynamic.Size2D();
        if (DynamicDistance<GetConfig().DynamicAvoidanceDistance)
        {
            DynamicAvoidance=FromDynamic.GetSafeNormal2D()*(1.f-DynamicDistance/FMath::Max(1.f,GetConfig().DynamicAvoidanceDistance));
        }
        if (ThreatDistance <= FMath::Max(1.f, C.NoticeDistance))
        {
            FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeThreatSight), false, &Horse);
            Params.AddIgnoredActor(Target);
            if (Target->GetAttachParentActor()) { Params.AddIgnoredActor(Target->GetAttachParentActor()); }
            FHitResult Hit;
            bThreatVisible = !GetWorld()->LineTraceSingleByChannel(Hit, Horse.GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Params);
        }
        if (bThreatVisible) { LastThreatPosition = Target->GetActorLocation(); }
    }
    UnseenSeconds = bThreatVisible ? 0.f : UnseenSeconds + Dt;
    // Player pressure is independent of the horse's own escape velocity: a fleeing
    // horse must not interpret its increasing separation as the player stopping.
    const bool bPressure = bThreatVisible && ApproachSpeed > C.ApproachDeadZone;
    ReleasedSeconds = bPressure ? 0.f : ReleasedSeconds + Dt;
    if (bPressure)
    {
        const float Proximity = 1.f - FMath::Clamp(ThreatDistance / FMath::Max(1.f, C.NoticeDistance), 0.f, 1.f);
        const float Rush = FMath::Clamp(ApproachSpeed / FMath::Max(1.f, C.FastClosingSpeed), 0.f, 2.f);
        const float Ceiling = ApproachSpeed >= C.FastClosingSpeed ? 1.f : FMath::Max(C.AlertThreshold,FMath::Min(.6f,C.FlightThreshold-.05f));
        const float Next = Awareness + FMath::Max(.01f,C.AwarenessRiseRate)*AwarenessRiseScale * (.25f + Proximity + Rush) * Dt;
        // Reduce existing fright gradually instead of snapping down to the slow cap.
        Awareness = Awareness > Ceiling ? FMath::Max(Ceiling,Awareness-C.AwarenessDecayRate*AwarenessDecayScale*Dt) : FMath::Min(Ceiling,Next);
    }
    else { Awareness = FMath::Max(0.f, Awareness - FMath::Max(.01f,C.AwarenessDecayRate)*AwarenessDecayScale * Dt); }
    HerdAlarmSeconds=FMath::Max(0.f,HerdAlarmSeconds-Dt);
    if (HerdAlarmSeconds>0.f) { Awareness=FMath::Max(Awareness,HerdAlarmStrength); }
}
void UHorseBrainComponent::ChangeState(EWildHorseState NewState)
{
    if (State == NewState) { return; }
    State = NewState;
    StateSeconds = 0.f;
    UE_LOG(LogSteppeHorse, Display, TEXT("%s behavior -> %s (awareness %.2f distance %.1f approach %.1f herdAlarm %.2f)"),
        *GetOwner()->GetName(),*UEnum::GetValueAsString(State),Awareness,ThreatDistance,ApproachSpeed,HerdAlarmSeconds);
    if (State == EWildHorseState::Roaming)
    {
        Home = GetOwner()->GetActorLocation();
        ChooseRoamGoal();
    }
}
void UHorseBrainComponent::ChooseRoamGoal()
{
    const float Angle = Random.FRandRange(-PI, PI);
    const float Radius = FMath::Max(0.f, GetConfig().RoamRadius) * Random.FRandRange(.3f,1.f);
    Goal = Home + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
    PauseRemaining = FMath::Max(0.f, GetConfig().PauseSeconds*IndividualPauseScale*Random.FRandRange(.8f,1.2f));
    RoamGoalSeconds = 0.f;
}
bool UHorseBrainComponent::IsDirectionSupported(const ASteppeHorseCharacter& Horse, FVector Direction, float Distance) const
{
    const auto& C = GetConfig();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeWildSteer), false, &Horse);
    const FVector Start = Horse.GetActorLocation();
    const float Length = FMath::Max(1.f, Distance);
    FHitResult Hit;
    FCollisionObjectQueryParams Environment;
    Environment.AddObjectTypesToQuery(ECC_WorldStatic);
    Environment.AddObjectTypesToQuery(ECC_WorldDynamic);
    if (GetWorld()->SweepSingleByObjectType(Hit, Start, Start+Direction*Length, FQuat::Identity, Environment,
        FCollisionShape::MakeSphere(FMath::Max(1.f,C.ProbeRadius)), Params)) { return false; }
    const int32 Samples = FMath::Clamp(FMath::CeilToInt(Length/FMath::Max(10.f,C.GroundSampleSpacing)),1,32);
    float PreviousGroundZ = Start.Z-Horse.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for (int32 Index=1; Index<=Samples; ++Index)
    {
        const FVector Sample = Start+Direction*(Length*Index/Samples);
        const FVector Top(Sample.X,Sample.Y,PreviousGroundZ+Horse.GetCharacterMovement()->MaxStepHeight+10.f);
        const FVector Bottom(Sample.X,Sample.Y,PreviousGroundZ-FMath::Max(1.f,C.GroundProbeDepth));
        if (!GetWorld()->LineTraceSingleByObjectType(Hit,Top,Bottom,Environment,Params)
            || Hit.ImpactNormal.Z<Horse.GetCharacterMovement()->GetWalkableFloorZ()
            || PreviousGroundZ-Hit.ImpactPoint.Z>FMath::Max(0.f,C.MaximumGroundDrop)) { return false; }
        PreviousGroundZ=Hit.ImpactPoint.Z;
    }
    return true;
}
FVector UHorseBrainComponent::FindSafeDirection(const ASteppeHorseCharacter& Horse, FVector Desired)
{
    const auto& C = GetConfig();
    const float ProbeLength = FMath::Max3<float>(C.ProbeDistance, Horse.GetVelocity().Size2D()*FMath::Max(.1f,C.ProbeSeconds),StoppingProbeDistance);
    float BestScore = -BIG_NUMBER;
    FVector Best = FVector::ZeroVector;
    // Reject unsafe intermediate ground, even when the far endpoint is supported.
    for (float Angle : {0.f,22.5f,-22.5f,45.f,-45.f,67.5f,-67.5f,90.f,-90.f,135.f,-135.f,180.f})
    {
        const FVector Direction = Desired.RotateAngleAxis(Angle, FVector::UpVector);
        const float Score = FVector::DotProduct(Direction, Desired) + .15f * FVector::DotProduct(Direction, SteeringDirection);
        if (Score > BestScore && IsDirectionSupported(Horse,Direction,ProbeLength)) { BestScore = Score; Best = Direction; }
    }
    bPathBlocked = Best.IsNearlyZero();
    return Best;
}
void UHorseBrainComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt, TickType, TickFunction);
    auto* Horse = Cast<ASteppeHorseCharacter>(GetOwner());
    auto* Movement = Horse ? Cast<UHorseMovementComponent>(Horse->GetCharacterMovement()) : nullptr;
    if (!Movement || Horse->MountedRider.IsValid() || Dt <= 0.f) { return; }
    if (bCaptured)
    {
        FHorseMovementIntent CapturedIntent;
        CapturedRetreatSeconds=FMath::Max(0.f,CapturedRetreatSeconds-Dt);
        if (bLeading && LeadTarget.IsValid())
        {
            const AActor* Leader=LeadTarget.Get();
            const FVector Anchor=Leader->GetActorLocation()-Leader->GetActorForwardVector()*LeadFollowDistance;
            const FVector ToAnchor=Anchor-Horse->GetActorLocation();
            LeadDistance=FVector::Dist2D(Horse->GetActorLocation(),Leader->GetActorLocation());
            if (LeadDistance<=LeadMaxDistance && ToAnchor.Size2D()>LeadMoveThreshold)
            {
                const FVector SafeDirection=FindSafeDirection(*Horse,ToAnchor.GetSafeNormal2D());
                if (!SafeDirection.IsNearlyZero())
                {
                    const float HeadingError=FMath::FindDeltaAngleDegrees(Horse->GetActorRotation().Yaw,SafeDirection.Rotation().Yaw);
                    CapturedIntent.DesiredSpeed=LeadWalkSpeed;
                    CapturedIntent.DesiredTurn=FMath::Clamp(HeadingError/45.f,-1.f,1.f);
                    CapturedIntent.RequestedGait=EHorseGait::Walk;
                    SteeringDirection=SafeDirection;
                }
                else { CapturedIntent.BrakeStrength=1.f; }
            }
            else
            {
                CapturedIntent.BrakeStrength=1.f;
                SteeringDirection=FVector::ZeroVector;
            }
        }
        else if (CapturedRetreatSeconds>0.f && !CapturedRetreatDirection.IsNearlyZero())
        {
            const float HeadingError=FMath::FindDeltaAngleDegrees(Horse->GetActorRotation().Yaw,CapturedRetreatDirection.Rotation().Yaw);
            CapturedIntent.DesiredSpeed=CapturedRetreatSpeed;
            CapturedIntent.DesiredTurn=FMath::Clamp(HeadingError/45.f,-1.f,1.f);
            CapturedIntent.RequestedGait=EHorseGait::Walk;
            SteeringDirection=CapturedRetreatDirection;
        }
        else
        {
            CapturedIntent.BrakeStrength=1.f;
            SteeringDirection=FVector::ZeroVector;
        }
        Movement->SetHorseIntent(CapturedIntent);
        return;
    }
    if (bLassoed)
    {
        const auto& C=GetConfig();
        FHorseMovementIntent StruggleIntent;
        FVector Away=(Horse->GetActorLocation()-LassoAnchor).GetSafeNormal2D();
        if (Away.IsNearlyZero()) { Away=Horse->GetActorForwardVector(); }
        const FVector SafeDirection=FindSafeDirection(*Horse,Away);
        if (!SafeDirection.IsNearlyZero())
        {
            const float HeadingError=FMath::FindDeltaAngleDegrees(Horse->GetActorRotation().Yaw,SafeDirection.Rotation().Yaw);
            const float CalmScale=FMath::Lerp(1.f,.12f,LassoControlProgress);
            StruggleIntent.DesiredSpeed=C.FlightSpeed*StruggleSpeedScale*CalmScale*LassoSpeedLimitScale;
            StruggleIntent.DesiredTurn=FMath::Clamp(HeadingError/FMath::Max(1.f,C.FullTurnAngle),-1.f,1.f);
            StruggleIntent.RequestedGait=StruggleIntent.DesiredSpeed>C.YieldSpeed*1.5f?EHorseGait::Gallop:EHorseGait::Walk;
            SteeringDirection=SafeDirection;
        }
        else
        {
            StruggleIntent.BrakeStrength=1.f;
            SteeringDirection=FVector::ZeroVector;
        }
        Movement->SetHorseIntent(StruggleIntent);
        return;
    }
    const auto& C = GetConfig();
    const float Speed = Horse->GetVelocity().Size2D();
    const float BrakeRate = FMath::Max(1.f,Horse->GetLocomotionConfig()->EmergencyBrakeRate);
    StoppingProbeDistance = Speed*Speed/(2.f*BrakeRate)+Speed*FMath::Max(.02f,C.DecisionInterval)+FMath::Max(0.f,C.BrakeSafetyDistance);
    bBrakingForHazard = Movement->IsMovingOnGround() && Speed>10.f
        && !IsDirectionSupported(*Horse,Horse->GetVelocity().GetSafeNormal2D(),StoppingProbeDistance);
    StateSeconds += Dt;
    Sense(Dt, *Horse);
    const bool ImmediateDanger = bThreatVisible && (ThreatDistance <= C.PanicDistance ||
        (ThreatDistance <= C.FastApproachDistance && ApproachSpeed >= C.FastClosingSpeed));
    const bool Alarm = ImmediateDanger || HerdAlarmSeconds>0.f || (ApproachSpeed >= C.FastClosingSpeed && Awareness >= C.FlightThreshold);
    const bool YieldPressure = bThreatVisible && ThreatDistance <= C.FlightDistance && ApproachSpeed > C.ApproachDeadZone;
    switch (State)
    {
    case EWildHorseState::Roaming:
        if (ImmediateDanger) { ChangeState(EWildHorseState::Fleeing); }
        else if (Awareness >= C.AlertThreshold) { ChangeState(EWildHorseState::Alert); }
        break;
    case EWildHorseState::Alert:
        if (ImmediateDanger || (Alarm && StateSeconds >= C.MinimumAlertSeconds*IndividualReactionScale)) { ChangeState(EWildHorseState::Fleeing); }
        else if (YieldPressure && StateSeconds >= C.MinimumAlertSeconds*IndividualReactionScale) { ChangeState(EWildHorseState::Yielding); }
        else if (Awareness <= C.CalmThreshold && StateSeconds >= C.MinimumAlertSeconds*IndividualReactionScale) { ChangeState(EWildHorseState::Recovering); }
        break;
    case EWildHorseState::Fleeing:
        if (!ImmediateDanger && ReleasedSeconds >= C.PressureReleaseSeconds && (bThreatVisible || UnseenSeconds >= C.ThreatMemorySeconds) && StateSeconds >= C.MinimumFlightSeconds) { ChangeState(EWildHorseState::Recovering); }
        break;
    case EWildHorseState::Yielding:
        if (Alarm) { ChangeState(EWildHorseState::Fleeing); }
        else if (!YieldPressure && StateSeconds >= C.MinimumAlertSeconds*IndividualReactionScale) { ChangeState(EWildHorseState::Alert); }
        break;
    case EWildHorseState::Recovering:
        if (ImmediateDanger) { ChangeState(EWildHorseState::Fleeing); }
        else if (bThreatVisible && ApproachSpeed > C.ApproachDeadZone && Awareness >= C.AlertThreshold) { ChangeState(EWildHorseState::Alert); }
        else if (Awareness <= C.CalmThreshold && StateSeconds >= C.RecoverySeconds) { ChangeState(EWildHorseState::Roaming); }
        break;
    case EWildHorseState::Lassoed:
    case EWildHorseState::Captured:
        break;
    }
    FHorseMovementIntent Intent;
    bPathBlocked = false;
    if (State == EWildHorseState::Fleeing || State == EWildHorseState::Yielding)
    {
        FVector Away = (Horse->GetActorLocation() - LastThreatPosition).GetSafeNormal2D();
        if (Away.IsNearlyZero()) { Away = Horse->GetActorForwardVector(); }
        Goal = Horse->GetActorLocation() + Away * FMath::Max(1.f,C.EscapeLookAhead);
        Intent.DesiredSpeed = State == EWildHorseState::Yielding ? C.YieldSpeed : C.FlightSpeed*FlightSpeedScale;
        Intent.RequestedGait = State == EWildHorseState::Yielding ? EHorseGait::Walk : EHorseGait::Gallop;
    }
    else if (State == EWildHorseState::Roaming)
    {
        RoamGoalSeconds += Dt;
        if (FVector::Dist2D(Horse->GetActorLocation(), Goal) <= C.ArrivalRadius || RoamGoalSeconds > C.RoamGoalTimeout) { ChooseRoamGoal(); }
        PauseRemaining = FMath::Max(0.f, PauseRemaining - Dt);
        Intent.DesiredSpeed = PauseRemaining > 0.f ? 0.f : C.RoamSpeed;
        Intent.RequestedGait = EHorseGait::Walk;
    }
    if (Intent.DesiredSpeed > 0.f)
    {
        FVector Desired = (Goal - Horse->GetActorLocation()).GetSafeNormal2D();
        if (HerdNeighborCount>0)
        {
            FVector Social=Desired+C.SeparationWeight*HerdSeparation+C.DynamicAvoidanceWeight*DynamicAvoidance;
            if (State==EWildHorseState::Fleeing && !bIsolationFocus)
            {
                Social+=C.FlightDirectionWeight*HerdEscapeDirection;
                Social+=C.FlightAlignmentWeight*HerdVelocity.GetSafeNormal2D();
                Social+=C.FlightCohesionWeight*(HerdCenter-Horse->GetActorLocation()).GetSafeNormal2D();
            }
            else if (!bIsolationFocus)
            {
                Social+=C.CohesionWeight*(HerdCenter-Horse->GetActorLocation()).GetSafeNormal2D();
                Social+=C.AlignmentWeight*HerdVelocity.GetSafeNormal2D();
            }
            if (!Social.IsNearlyZero()) { Desired=Social.GetSafeNormal2D(); }
        }
        else if (!DynamicAvoidance.IsNearlyZero())
        {
            Desired=(Desired+C.DynamicAvoidanceWeight*DynamicAvoidance).GetSafeNormal2D();
        }
        const float BiasScale=State==EWildHorseState::Fleeing && !bIsolationFocus?.3f:1.f;
        Desired=Desired.RotateAngleAxis(IndividualSteeringBias*BiasScale,FVector::UpVector);
        SteeringDirection = FindSafeDirection(*Horse, Desired);
        const float HeadingError = FMath::FindDeltaAngleDegrees(Horse->GetActorRotation().Yaw, SteeringDirection.Rotation().Yaw);
        Intent.DesiredTurn = FMath::Clamp(HeadingError / FMath::Max(1.f,C.FullTurnAngle), -1.f, 1.f);
        // Turn before charging away when the safe direction is behind us. Existing momentum still brakes through CMC.
        Intent.DesiredSpeed *= FMath::Clamp(FVector::DotProduct(Horse->GetActorForwardVector(), SteeringDirection), 0.f, 1.f);
        if (bPathBlocked && RecoveryTurnRemaining<=0.f) { RecoveryTurnRemaining=C.BlockedTurnSeconds; }
    }
    else { SteeringDirection = FVector::ZeroVector; }
    RecoveryTurnRemaining=FMath::Max(0.f,RecoveryTurnRemaining-Dt);
    bRecoveringFromBlockage=RecoveryTurnRemaining>0.f;
    if (bRecoveringFromBlockage)
    {
        Intent.DesiredSpeed=0.f;
        Intent.DesiredTurn=IndividualSteeringBias>=0.f?1.f:-1.f;
        Intent.BrakeStrength=1.f;
    }
    if (bBrakingForHazard) { Intent.DesiredSpeed=0.f; Intent.BrakeStrength=1.f; }
    Movement->SetHorseIntent(Intent);
}
FGameplayTag UHorseBrainComponent::GetBehaviorTag() const
{
    switch (State)
    {
    case EWildHorseState::Yielding: return SteppeTags::Horse_State_Yielding;
    case EWildHorseState::Alert: return SteppeTags::Horse_State_Alert;
    case EWildHorseState::Fleeing: return SteppeTags::Horse_State_Flee;
    case EWildHorseState::Recovering: return SteppeTags::Horse_State_Recovering;
    case EWildHorseState::Lassoed: return SteppeTags::Horse_State_Lassoed;
    case EWildHorseState::Captured: return SteppeTags::Horse_State_Captured;
    default: return SteppeTags::Horse_State_Roaming;
    }
}
