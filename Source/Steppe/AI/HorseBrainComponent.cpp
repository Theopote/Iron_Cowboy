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
    Random.Initialize(GetConfig().RandomSeed);
    SetComponentTickInterval(FMath::Max(.02f, GetConfig().DecisionInterval));
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
void UHorseBrainComponent::Sense(float Dt, const ASteppeHorseCharacter& Horse)
{
    const auto& C = GetConfig();
    bThreatVisible = false;
    ThreatDistance = 0.f;
    ClosingSpeed = 0.f;
    ApproachSpeed = 0.f;
    if (AActor* Target = ThreatTarget.Get())
    {
        const FVector ToHorse = (Horse.GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
        ThreatDistance = FVector::Dist2D(Horse.GetActorLocation(), Target->GetActorLocation());
        // An attached rider's own movement component is disabled; use the mount's velocity.
        const AActor* MotionSource = Target->GetAttachParentActor() ? Target->GetAttachParentActor() : Target;
        ApproachSpeed = FVector::DotProduct(MotionSource->GetVelocity(), ToHorse);
        ClosingSpeed = FMath::Max(0.f, FVector::DotProduct(MotionSource->GetVelocity() - Horse.GetVelocity(), ToHorse));
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
        const float Next = Awareness + FMath::Max(.01f,C.AwarenessRiseRate) * (.25f + Proximity + Rush) * Dt;
        // Reduce existing fright gradually instead of snapping down to the slow cap.
        Awareness = Awareness > Ceiling ? FMath::Max(Ceiling,Awareness-C.AwarenessDecayRate*Dt) : FMath::Min(Ceiling,Next);
    }
    else { Awareness = FMath::Max(0.f, Awareness - FMath::Max(.01f,C.AwarenessDecayRate) * Dt); }
}
void UHorseBrainComponent::ChangeState(EWildHorseState NewState)
{
    if (State == NewState) { return; }
    State = NewState;
    StateSeconds = 0.f;
    UE_LOG(LogSteppeHorse, Display, TEXT("%s behavior -> %s (awareness %.2f)"), *GetOwner()->GetName(), *UEnum::GetValueAsString(State), Awareness);
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
    PauseRemaining = FMath::Max(0.f, GetConfig().PauseSeconds);
    RoamGoalSeconds = 0.f;
}
bool UHorseBrainComponent::IsDirectionSupported(const ASteppeHorseCharacter& Horse, FVector Direction, float Distance) const
{
    const auto& C = GetConfig();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeWildSteer), false, &Horse);
    const FVector Start = Horse.GetActorLocation();
    const float Length = FMath::Max(1.f, Distance);
    FHitResult Hit;
    if (GetWorld()->SweepSingleByChannel(Hit, Start, Start+Direction*Length, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeSphere(FMath::Max(1.f,C.ProbeRadius)), Params)) { return false; }
    const int32 Samples = FMath::Clamp(FMath::CeilToInt(Length/FMath::Max(10.f,C.GroundSampleSpacing)),1,32);
    float PreviousGroundZ = Start.Z-Horse.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for (int32 Index=1; Index<=Samples; ++Index)
    {
        const FVector Sample = Start+Direction*(Length*Index/Samples);
        const FVector Top(Sample.X,Sample.Y,PreviousGroundZ+Horse.GetCharacterMovement()->MaxStepHeight+10.f);
        const FVector Bottom(Sample.X,Sample.Y,PreviousGroundZ-FMath::Max(1.f,C.GroundProbeDepth));
        if (!GetWorld()->LineTraceSingleByChannel(Hit,Top,Bottom,ECC_Visibility,Params)
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
    for (float Angle : {0.f,45.f,-45.f,90.f,-90.f,135.f,-135.f,180.f})
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
    const bool Alarm = ImmediateDanger || (ApproachSpeed >= C.FastClosingSpeed && Awareness >= C.FlightThreshold);
    const bool YieldPressure = bThreatVisible && ThreatDistance <= C.FlightDistance && ApproachSpeed > C.ApproachDeadZone;
    switch (State)
    {
    case EWildHorseState::Roaming:
        if (ImmediateDanger) { ChangeState(EWildHorseState::Fleeing); }
        else if (Awareness >= C.AlertThreshold) { ChangeState(EWildHorseState::Alert); }
        break;
    case EWildHorseState::Alert:
        if (ImmediateDanger || (Alarm && StateSeconds >= C.MinimumAlertSeconds)) { ChangeState(EWildHorseState::Fleeing); }
        else if (YieldPressure && StateSeconds >= C.MinimumAlertSeconds) { ChangeState(EWildHorseState::Yielding); }
        else if (Awareness <= C.CalmThreshold && StateSeconds >= C.MinimumAlertSeconds) { ChangeState(EWildHorseState::Recovering); }
        break;
    case EWildHorseState::Fleeing:
        if (!ImmediateDanger && ReleasedSeconds >= C.PressureReleaseSeconds && (bThreatVisible || UnseenSeconds >= C.ThreatMemorySeconds) && StateSeconds >= C.MinimumFlightSeconds) { ChangeState(EWildHorseState::Recovering); }
        break;
    case EWildHorseState::Yielding:
        if (Alarm) { ChangeState(EWildHorseState::Fleeing); }
        else if (!YieldPressure && StateSeconds >= C.MinimumAlertSeconds) { ChangeState(EWildHorseState::Alert); }
        break;
    case EWildHorseState::Recovering:
        if (ImmediateDanger) { ChangeState(EWildHorseState::Fleeing); }
        else if (bThreatVisible && ApproachSpeed > C.ApproachDeadZone && Awareness >= C.AlertThreshold) { ChangeState(EWildHorseState::Alert); }
        else if (Awareness <= C.CalmThreshold && StateSeconds >= C.RecoverySeconds) { ChangeState(EWildHorseState::Roaming); }
        break;
    }
    FHorseMovementIntent Intent;
    bPathBlocked = false;
    if (State == EWildHorseState::Fleeing || State == EWildHorseState::Yielding)
    {
        FVector Away = (Horse->GetActorLocation() - LastThreatPosition).GetSafeNormal2D();
        if (Away.IsNearlyZero()) { Away = Horse->GetActorForwardVector(); }
        Goal = Horse->GetActorLocation() + Away * FMath::Max(1.f,C.EscapeLookAhead);
        Intent.DesiredSpeed = State == EWildHorseState::Yielding ? C.YieldSpeed : C.FlightSpeed;
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
        const FVector Desired = (Goal - Horse->GetActorLocation()).GetSafeNormal2D();
        SteeringDirection = FindSafeDirection(*Horse, Desired);
        const float HeadingError = FMath::FindDeltaAngleDegrees(Horse->GetActorRotation().Yaw, SteeringDirection.Rotation().Yaw);
        Intent.DesiredTurn = FMath::Clamp(HeadingError / FMath::Max(1.f,C.FullTurnAngle), -1.f, 1.f);
        // Turn before charging away when the safe direction is behind us. Existing momentum still brakes through CMC.
        Intent.DesiredSpeed *= FMath::Clamp(FVector::DotProduct(Horse->GetActorForwardVector(), SteeringDirection), 0.f, 1.f);
        if (bPathBlocked) { Intent.DesiredSpeed = 0.f; Intent.DesiredTurn = 0.f; Intent.BrakeStrength = 1.f; }
    }
    else { SteeringDirection = FVector::ZeroVector; }
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
    default: return SteppeTags::Horse_State_Roaming;
    }
}
