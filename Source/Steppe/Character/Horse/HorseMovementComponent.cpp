#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "Character/Horse/HorseLocomotionMath.h"

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
}
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
    }
    // The response boundary also accepts future non-rider intents. No AI is implemented here.
    float Limit=FMath::Max(0.f,A.MaxSpeed);
    if (bExhausted) { Limit=FMath::Min(Limit,FMath::Max(0.f,C.GetGait(EHorseGait::Gallop).TargetSpeed)); }
    DesiredSpeed=HorseIntent.BrakeStrength>0.f?0.f:FMath::Clamp(HorseIntent.DesiredSpeed*FMath::Clamp(SurfaceMovementMultiplier,0.f,2.f),0.f,Limit);
    MaxWalkSpeed=FMath::Max(1.f,A.MaxSpeed);
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
    Velocity=Heading.Vector()*Speed;
    Acceleration=Heading.Vector()*(DesiredSpeed>Speed?Accel:-Brake);
}
void UHorseMovementComponent::PhysicsRotation(float DeltaTime)
{
    // Ground yaw is integrated with velocity in CalcVelocity. Airborne heading is preserved.
}
