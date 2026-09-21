#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "Character/Horse/HorseLocomotionMath.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

UHorseMovementComponent::UHorseMovementComponent()
{
    bRunPhysicsWithNoController = true;
    bOrientRotationToMovement = false;
    bUseControllerDesiredRotation = false;
    bAllowPhysicsRotationDuringAnimRootMotion = false;
    MaxWalkSpeed = 1500.f;
    GroundFriction = 0.f;
    bUseSeparateBrakingFriction = true;
    BrakingFriction = 0.f;
}
void UHorseMovementComponent::SetRiderIntent(const FRidingIntent& Intent) { RiderIntent=Intent; RiderIntent.Clamp(); bRiderSource=true; }
void UHorseMovementComponent::SetHorseIntent(const FHorseMovementIntent& Intent)
{
    HorseIntent=Intent;
    HorseIntent.DesiredSpeed=FMath::IsFinite(Intent.DesiredSpeed)?FMath::Max(0.f,Intent.DesiredSpeed):0.f;
    HorseIntent.DesiredTurn=FMath::IsFinite(Intent.DesiredTurn)?FMath::Clamp(Intent.DesiredTurn,-1.f,1.f):0.f;
    HorseIntent.BrakeStrength=FMath::IsFinite(Intent.BrakeStrength)?FMath::Clamp(Intent.BrakeStrength,0.f,1.f):0.f;
    bRiderSource=false;
}
void UHorseMovementComponent::ClearIntent()
{
    RiderIntent.Reset(); HorseIntent=FHorseMovementIntent(); ResponseForward=ResponseTurn=0.f; bRiderSource=true;
    bRiderAvoidingObstacle=false; RiderAvoidanceTurn=0.f; RiderObstacleDistance=0.f; RiderAvoidanceSpeedScale=1.f;
}
void UHorseMovementComponent::SetExternalAcceleration(FVector InAcceleration)
{
    ExternalAcceleration=FMath::IsFinite(InAcceleration.X) && FMath::IsFinite(InAcceleration.Y)
        ?FVector(InAcceleration.X,InAcceleration.Y,0.f).GetClampedToMaxSize(FMath::Max(0.f,MaximumExternalAcceleration))
        :FVector::ZeroVector;
}
void UHorseMovementComponent::ClearExternalAcceleration() { ExternalAcceleration=FVector::ZeroVector; }
void UHorseMovementComponent::UpdateResponse(float Dt, ASteppeHorseCharacter& Horse)
{
    const auto& C=*Horse.GetLocomotionConfig();
    auto& A=*Horse.Attributes;
    if (A.CurrentStamina <= C.ExhaustionThreshold) { bExhausted=true; }
    else if (A.CurrentStamina >= FMath::Max(C.ExhaustionThreshold+1.f,C.SprintResumeThreshold)) { bExhausted=false; }
    if (bRiderSource)
    {
        const float Step=Dt/FMath::Max(.01f,C.ResponseSeconds);
        ResponseForward=FMath::FInterpConstantTo(ResponseForward,FMath::Max(0.f,RiderIntent.Forward),Dt,1.f/FMath::Max(.01f,C.ResponseSeconds));
        ResponseTurn+=FMath::Clamp(RiderIntent.Turn-ResponseTurn,-Step,Step);
        HorseIntent.DesiredTurn=ResponseTurn;
        HorseIntent.BrakeStrength=(RiderIntent.bBrake || RiderIntent.Forward<0.f)?1.f:0.f;
        const EHorseGait Request=(RiderIntent.bSprint && !bExhausted)?EHorseGait::Sprint:EHorseGait::Gallop;
        HorseIntent.DesiredSpeed=ResponseForward*FMath::Max(0.f,C.GetGait(Request).TargetSpeed);
        HorseIntent.RequestedGait=SteppeHorseMath::SelectGait(HorseIntent.DesiredSpeed,HorseIntent.RequestedGait,C);
        ApplyRiderObstacleAvoidance(Horse);
    }
    else
    {
        bRiderAvoidingObstacle=false;
        RiderAvoidanceTurn=0.f;
        RiderObstacleDistance=0.f;
        RiderAvoidanceSpeedScale=1.f;
    }
    // The response boundary also accepts future non-rider intents. No AI is implemented here.
    float Limit=FMath::Max(0.f,A.MaxSpeed);
    if (bExhausted) { Limit=FMath::Min(Limit,FMath::Max(0.f,C.GetGait(EHorseGait::Gallop).TargetSpeed)); }
    DesiredSpeed=HorseIntent.BrakeStrength>0.f?0.f:FMath::Clamp(HorseIntent.DesiredSpeed*FMath::Clamp(SurfaceMovementMultiplier,0.f,2.f),0.f,Limit);
    MaxWalkSpeed=FMath::Max(1.f,A.MaxSpeed);
}

bool UHorseMovementComponent::ProbeRiderPath(ASteppeHorseCharacter& Horse, FVector Direction, float Distance, FHitResult* OutHit) const
{
    UWorld* World=Horse.GetWorld();
    if (!World || Direction.IsNearlyZero()) { return false; }
    const float HalfHeight=Horse.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Start=Horse.GetActorLocation()+FVector(0,0,FMath::Min(80.f,HalfHeight*.65f));
    const FVector End=Start+Direction.GetSafeNormal2D()*Distance;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeRiderAvoidance),false,&Horse);
    if (Horse.MountedRider.IsValid()) { Params.AddIgnoredActor(Horse.MountedRider.Get()); }
    FHitResult LocalHit;
    const bool bHit=World->SweepSingleByChannel(LocalHit,Start,End,FQuat::Identity,ECC_Visibility,
        FCollisionShape::MakeSphere(RiderObstacleProbeRadius),Params);
    if (OutHit) { *OutHit=LocalHit; }
    return bHit;
}

void UHorseMovementComponent::ApplyRiderObstacleAvoidance(ASteppeHorseCharacter& Horse)
{
    bRiderAvoidingObstacle=false;
    RiderAvoidanceTurn=0.f;
    RiderObstacleDistance=0.f;
    RiderAvoidanceSpeedScale=1.f;
    if (HorseIntent.DesiredSpeed<10.f || RiderIntent.Forward<=0.f) { return; }

    const float SpeedProbe=FMath::Clamp(Velocity.Size2D()*.25f,0.f,RiderObstacleProbeDistance*.6f);
    const float ProbeDistance=RiderObstacleProbeDistance+SpeedProbe;
    const FVector Intended=Horse.GetActorForwardVector().RotateAngleAxis(RiderIntent.Turn*15.f,FVector::UpVector).GetSafeNormal2D();
    FHitResult CenterHit;
    if (!ProbeRiderPath(Horse,Intended,ProbeDistance,&CenterHit)) { return; }

    const FVector Left=Intended.RotateAngleAxis(-RiderAvoidanceAngle,FVector::UpVector);
    const FVector Right=Intended.RotateAngleAxis(RiderAvoidanceAngle,FVector::UpVector);
    const bool bLeftClear=!ProbeRiderPath(Horse,Left,ProbeDistance*.85f);
    const bool bRightClear=!ProbeRiderPath(Horse,Right,ProbeDistance*.85f);
    float Assist=PreviousAvoidanceTurn;
    if (RiderIntent.Turn<-.15f && bLeftClear) { Assist=-1.f; }
    else if (RiderIntent.Turn>.15f && bRightClear) { Assist=1.f; }
    else if (bLeftClear!=bRightClear) { Assist=bRightClear?1.f:-1.f; }
    else if (!bLeftClear && !bRightClear) { Assist=CenterHit.ImpactNormal.Dot(Horse.GetActorRightVector())>0.f?-1.f:1.f; }
    PreviousAvoidanceTurn=Assist;
    RiderAvoidanceTurn=Assist;
    RiderObstacleDistance=ProbeDistance*CenterHit.Time;
    bRiderAvoidingObstacle=true;
    const float AvoidanceBlend=RiderAvoidanceStrength*FMath::Lerp(.35f,1.f,1.f-FMath::Abs(HorseIntent.DesiredTurn));
    HorseIntent.DesiredTurn=FMath::Clamp(FMath::Lerp(HorseIntent.DesiredTurn,Assist,AvoidanceBlend),-1.f,1.f);
    RiderAvoidanceSpeedScale=FMath::Lerp(.35f,.8f,FMath::Clamp(CenterHit.Time,0.f,1.f));
    HorseIntent.DesiredSpeed*=RiderAvoidanceSpeedScale;
    if (!bLeftClear && !bRightClear) { HorseIntent.BrakeStrength=FMath::Max(HorseIntent.BrakeStrength,.65f); }
}
void UHorseMovementComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    ASteppeHorseCharacter* Horse=Cast<ASteppeHorseCharacter>(CharacterOwner);
    if (!Horse || !Horse->Attributes || Dt<=0.f) { Super::TickComponent(Dt,TickType,TickFunction); return; }
    const float Before=Velocity.Size2D();
    UpdateResponse(Dt,*Horse);
    Super::TickComponent(Dt,TickType,TickFunction);
    const auto& C=*Horse->GetLocomotionConfig();
    auto& A=*Horse->Attributes;
    CurrentSpeed=Velocity.Size2D();
    CurrentHeading=Horse->GetActorRotation().Yaw;
    const FVector Facing=Horse->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right=Horse->GetActorRightVector().GetSafeNormal2D();
    ForwardSpeed=FVector::DotProduct(Velocity,Facing);
    LateralSpeed=FVector::DotProduct(Velocity,Right);
    SlipAngleDegrees=CurrentSpeed>1.f?FMath::FindDeltaAngleDegrees(CurrentHeading,Velocity.Rotation().Yaw):0.f;
    ActualAcceleration=(CurrentSpeed-Before)/Dt;
    Gait=SteppeHorseMath::SelectGait(CurrentSpeed,Gait,C);
    HorseState=MovementMode==MOVE_None?EHorseMovementState::Disabled:(IsFalling()?EHorseMovementState::Falling:EHorseMovementState::Grounded);
    GroundSlope=CurrentFloor.IsWalkableFloor()?FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CurrentFloor.HitResult.ImpactNormal.Z,-1.f,1.f))):0.f;
    A.UpdateStamina(Dt,C.GetGait(Gait).StaminaMultiplier);
    auto& Data=Horse->AnimationData;
    Data.Speed=CurrentSpeed;
    Data.NormalizedSpeed=FMath::Clamp(CurrentSpeed/FMath::Max(1.f,A.MaxSpeed),0.f,1.f);
    Data.Gait=Gait;
    Data.NormalizedAcceleration=FMath::Clamp(ActualAcceleration/FMath::Max(1.f,ActualAcceleration>=0?A.Acceleration:C.EmergencyBrakeRate),-1.f,1.f);
    Data.AccelerationAmount=FMath::Max(0.f,Data.NormalizedAcceleration);
    Data.DecelerationAmount=FMath::Max(0.f,-Data.NormalizedAcceleration);
    Data.TurnAmount=HorseIntent.DesiredTurn;
    Data.LeanAmount=FMath::Clamp(HorseIntent.DesiredTurn*Data.NormalizedSpeed,-1.f,1.f);
    Data.SlipAmount=FMath::Clamp(SlipAngleDegrees/25.f,-1.f,1.f);
    Data.ExternalForceAmount=FMath::Clamp(ExternalAcceleration.Size2D()/FMath::Max(1.f,MaximumExternalAcceleration),0.f,1.f);
    Data.bStarting=Data.AccelerationAmount>.3f && CurrentSpeed>20.f && CurrentSpeed<400.f;
    Data.bStopping=Data.DecelerationAmount>.35f && CurrentSpeed<500.f;
    Data.bStruggling=Data.ExternalForceAmount>.15f && CurrentSpeed<900.f;
    Data.IsGrounded=IsMovingOnGround();
    Data.IsStumbling=HorseState==EHorseMovementState::Stumbling;
    Data.StaminaNormalized=A.GetStaminaNormalized();
    TurnStress=FMath::Max(0.f,Data.NormalizedSpeed*FMath::Abs(HorseIntent.DesiredTurn)*C.TurnDifficulty);
}
void UHorseMovementComponent::CalcVelocity(float Dt,float Friction,bool bFluid,float BrakeDeceleration)
{
    auto* Horse=Cast<ASteppeHorseCharacter>(CharacterOwner);
    if (!Horse || !Horse->Attributes || !IsMovingOnGround()) { Super::CalcVelocity(Dt,Friction,bFluid,BrakeDeceleration); return; }
    const auto& C=*Horse->GetLocomotionConfig();
    const auto& A=*Horse->Attributes;
    const auto& Settings=C.GetGait(Gait==EHorseGait::Idle?EHorseGait::Walk:Gait);
    const float Accel=FMath::Max(1.f,FMath::Min(A.Acceleration,Settings.Acceleration));
    const float NaturalBrake=FMath::Max(1.f,FMath::Min(A.Deceleration,Settings.Deceleration));
    const float Brake=FMath::Lerp(NaturalBrake,FMath::Max(NaturalBrake,C.EmergencyBrakeRate),HorseIntent.BrakeStrength);
    const float Speed=SteppeHorseMath::ApproachSpeed(Velocity.Size2D(),DesiredSpeed,Accel,Brake,Dt);
    const float Normalized=FMath::Clamp(Speed/FMath::Max(1.f,A.MaxSpeed),0.f,1.f);
    EffectiveTurnRate=FMath::Max(0.f,A.BaseTurnRate*A.Agility*C.SpeedTurnCurve.GetRichCurveConst()->Eval(Normalized)*Settings.TurnMultiplier);
    CurrentHeading=UpdatedComponent->GetComponentRotation().Yaw;
    DesiredHeading=FRotator::NormalizeAxis(CurrentHeading+HorseIntent.DesiredTurn*C.HeadingLookAhead);
    const float Delta=FMath::Clamp(FMath::FindDeltaAngleDegrees(CurrentHeading,DesiredHeading),-EffectiveTurnRate*Dt*FMath::Abs(HorseIntent.DesiredTurn),EffectiveTurnRate*Dt*FMath::Abs(HorseIntent.DesiredTurn));
    const FRotator Heading(0,CurrentHeading+Delta,0);
    // Rotation belongs to the bounded horse response, never to an input callback.
    MoveUpdatedComponent(FVector::ZeroVector,Heading.Quaternion(),true);
    const float GripAlpha=FMath::Clamp(Normalized,0.f,1.f);
    EffectiveGripRate=FMath::Lerp(LowSpeedGripRate,HighSpeedGripRate,GripAlpha)*GetSurfaceGripMultiplier();
    const FVector Horizontal=SteppeHorseMath::TurnVelocityTowardFacing(Velocity,Heading.Vector(),Speed,EffectiveGripRate,Dt);
    const FVector Forced=(Horizontal+ExternalAcceleration*Dt).GetClampedToMaxSize(FMath::Max(1.f,A.MaxSpeed)*1.1f);
    Velocity=FVector(Forced.X,Forced.Y,Velocity.Z);
    Acceleration=Heading.Vector()*(DesiredSpeed>Speed?Accel:-Brake);
}
float UHorseMovementComponent::GetSurfaceGripMultiplier() const
{
    const FHitResult& FloorHit=CurrentFloor.HitResult;
    const UPhysicalMaterial* Material=FloorHit.PhysMaterial.Get();
    // CharacterMovement's floor sweep does not request physical material by default.
    if (!Material && FloorHit.GetComponent())
    {
        const UMaterialInterface* FloorMaterial=FloorHit.GetComponent()->GetMaterial(0);
        Material=FloorMaterial?FloorMaterial->GetPhysicalMaterial():nullptr;
    }
    return Material && UPhysicalMaterial::DetermineSurfaceType(Material)==SurfaceType2?HardGripMultiplier:GrassGripMultiplier;
}
void UHorseMovementComponent::PhysicsRotation(float DeltaTime)
{
    // Ground yaw is integrated with velocity in CalcVelocity. Airborne heading is preserved.
}
