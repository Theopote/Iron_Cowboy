#include "Character/Rider/RidingCameraComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
URidingCameraComponent::URidingCameraComponent() { PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickGroup=TG_PostPhysics; }
void URidingCameraComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
    {
        Rider->CameraBoom->PrimaryComponentTick.TickGroup=TG_PostPhysics;
        Rider->CameraBoom->AddTickPrerequisiteComponent(this);
    }
}
void URidingCameraComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()); if (!Rider || !Rider->Camera || !Rider->CameraBoom) { return; }
    auto* Horse=Rider->Riding->GetHorse();
    const auto& C=*(Horse?Horse->GetLocomotionConfig():GetDefault<UHorseLocomotionConfig>());
    const float Speed=Horse?Horse->AnimationData.Speed:0.f;
    const float Alpha=1.f-FMath::Exp(-FMath::Max(.01f,C.CameraBlendRate)*Dt);
    const float TargetFOV=FMath::Clamp(C.SpeedFOVCurve.GetRichCurveConst()->Eval(Speed),40.f,110.f);
    float TargetDistance=C.SpeedDistanceCurve.GetRichCurveConst()->Eval(Speed);
    if (Horse) { TargetDistance+=Horse->AnimationData.NormalizedAcceleration*C.AccelerationDistance; }
    Rider->Camera->SetFieldOfView(FMath::Lerp(Rider->Camera->FieldOfView,TargetFOV,Alpha));
    Rider->CameraBoom->TargetArmLength=FMath::Lerp(Rider->CameraBoom->TargetArmLength,FMath::Max(100.f,TargetDistance),Alpha);
    Rider->CameraBoom->CameraLagSpeed=FMath::Max(1.f,C.CameraLagSpeed);
    const float Offset=Horse?Horse->AnimationData.LeanAmount*C.TurnCameraOffset:0.f;
    Rider->CameraBoom->SocketOffset.Y=FMath::Lerp(Rider->CameraBoom->SocketOffset.Y,Offset,Alpha);
}
