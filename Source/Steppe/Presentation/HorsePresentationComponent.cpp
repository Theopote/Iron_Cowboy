#include "Presentation/HorsePresentationComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "UObject/ConstructorHelpers.h"

UHorsePresentationComponent::UHorsePresentationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseIdle.HorseIdle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseWalk.HorseWalk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Gallop(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseGallop.HorseGallop"));
    TemporaryIdleAnimation=Idle.Object;
    TemporaryWalkAnimation=Walk.Object;
    TemporaryGallopAnimation=Gallop.Object;
}

void UHorsePresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    if (const auto* Horse=Cast<ASteppeHorseCharacter>(GetOwner()))
    {
        if (const auto* Mesh=Horse->GetPlaceholderRoot())
        {
            BaseLocation=Mesh->GetRelativeLocation();
            BaseRotation=Mesh->GetRelativeRotation();
        }
        if (auto* AnimatedMesh=Horse->GetMesh(); AnimatedMesh && AnimatedMesh->GetSkeletalMeshAsset() && TemporaryIdleAnimation)
        {
            AnimatedMesh->PlayAnimation(TemporaryIdleAnimation,true);
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

    if (auto* AnimatedMesh=Horse->GetMesh(); AnimatedMesh && AnimatedMesh->GetSkeletalMeshAsset())
    {
        UAnimSequence* DesiredAnimation=TemporaryIdleAnimation;
        float PlayRate=1.f;
        if (Data.Gait==EHorseGait::Walk || Data.Gait==EHorseGait::Trot)
        {
            DesiredAnimation=TemporaryWalkAnimation;
            PlayRate=FMath::Clamp(Data.Speed/260.f,.65f,1.6f);
        }
        else if (Data.Gait!=EHorseGait::Idle)
        {
            DesiredAnimation=TemporaryGallopAnimation;
            PlayRate=FMath::Clamp(Data.Speed/1100.f,.7f,1.5f);
        }
        if (DesiredAnimation)
        {
            auto* Instance=AnimatedMesh->GetSingleNodeInstance();
            if (!Instance || Instance->GetAnimationAsset()!=DesiredAnimation)
            {
                AnimatedMesh->PlayAnimation(DesiredAnimation,true);
                Instance=AnimatedMesh->GetSingleNodeInstance();
            }
            if (Instance) { Instance->SetPlayRate(PlayRate); }
        }
    }

    if (auto* Mesh=Horse->GetPlaceholderRoot())
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
        Horse->AnimatePlaceholderLegs(Data.GaitPhase,bAnimatePlaceholder?Data.StrideBlend:0.f);
    }
}
