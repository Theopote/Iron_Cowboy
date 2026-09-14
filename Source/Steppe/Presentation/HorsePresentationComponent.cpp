#include "Presentation/HorsePresentationComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Components/StaticMeshComponent.h"

UHorsePresentationComponent::UHorsePresentationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}

void UHorsePresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    if (const auto* Horse=Cast<ASteppeHorseCharacter>(GetOwner()))
    {
        if (const auto* Mesh=Horse->GetPlaceholderMesh())
        {
            BaseLocation=Mesh->GetRelativeLocation();
            BaseRotation=Mesh->GetRelativeRotation();
        }
    }
}

float UHorsePresentationComponent::GetCycleFrequency(EHorseGait Gait) const
{
    switch (Gait)
    {
    case EHorseGait::Walk: return 1.35f;
    case EHorseGait::Trot: return 2.15f;
    case EHorseGait::Canter: return 2.7f;
    case EHorseGait::Gallop: return 3.35f;
    case EHorseGait::Sprint: return 4.1f;
    default: return .28f;
    }
}

void UHorsePresentationComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    auto* Horse=Cast<ASteppeHorseCharacter>(GetOwner());
    if (!Horse || Dt<=0.f) { return; }
    auto& Data=Horse->AnimationData;
    const float TargetStride=Data.IsGrounded?Data.NormalizedSpeed:0.f;
    Data.StrideBlend=FMath::FInterpTo(Data.StrideBlend,TargetStride,Dt,PoseResponse);
    PreviousPhase=Data.GaitPhase;
    Data.GaitPhase=FMath::Fmod(Data.GaitPhase+GetCycleFrequency(Data.Gait)*Dt,1.f);
    const bool bContact=(PreviousPhase<.5f && Data.GaitPhase>=.5f) || Data.GaitPhase<PreviousPhase;
    Data.FootContactPulse=bContact?1.f:FMath::Max(0.f,Data.FootContactPulse-Dt*8.f);
    const float Cycle=Data.GaitPhase*2.f*PI;
    const float IdleBreath=Data.Gait==EHorseGait::Idle?FMath::Sin(Cycle)*1.2f:0.f;
    const float TargetBob=IdleBreath+FMath::Sin(Cycle*2.f)*MaximumBob*Data.StrideBlend;
    const float TargetPitch=-Data.NormalizedAcceleration*AccelerationPitchDegrees+FMath::Cos(Cycle*2.f)*1.5f*Data.StrideBlend;
    const float TargetRoll=-Data.LeanAmount*MaximumLeanDegrees;
    Data.BodyBob=FMath::FInterpTo(Data.BodyBob,TargetBob,Dt,PoseResponse);
    Data.BodyPitch=FMath::FInterpTo(Data.BodyPitch,TargetPitch,Dt,PoseResponse);
    Data.BodyRoll=FMath::FInterpTo(Data.BodyRoll,TargetRoll,Dt,PoseResponse);

    if (auto* Mesh=Horse->GetPlaceholderMesh())
    {
        if (bAnimatePlaceholder)
        {
            Mesh->SetRelativeLocationAndRotation(BaseLocation+FVector(0,0,Data.BodyBob),
                BaseRotation+FRotator(Data.BodyPitch,0,Data.BodyRoll));
        }
        else
        {
            Mesh->SetRelativeLocationAndRotation(BaseLocation,BaseRotation);
        }
    }
}
