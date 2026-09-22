#include "Presentation/HorsePresentationComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Presentation/HorseAnimInstance.h"
#include "UObject/ConstructorHelpers.h"

UHorsePresentationComponent::UHorsePresentationComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseIdle.HorseIdle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseWalk.HorseWalk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Gallop(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseGallop.HorseGallop"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Stop(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseIdle_2.HorseIdle_2"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Struggle(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseIdle_HitReact1.HorseIdle_HitReact1"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> StruggleAlternate(TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseIdle_HitReact2.HorseIdle_HitReact2"));
    TemporaryIdleAnimation=Idle.Object;
    TemporaryWalkAnimation=Walk.Object;
    TemporaryGallopAnimation=Gallop.Object;
    TemporaryStopAnimation=Stop.Object;
    TemporaryStruggleAnimation=Struggle.Object;
    TemporaryStruggleAlternateAnimation=StruggleAlternate.Object;
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
        if (const auto* Skeletal=Horse->GetMesh())
        {
            SkeletalBaseLocation=Skeletal->GetRelativeLocation();
            SkeletalBaseRotation=Skeletal->GetRelativeRotation();
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
    const float PullAlpha=Data.bStruggling?1.f:0.f;
    const float TargetPitch=-Data.NormalizedAcceleration*AccelerationPitchDegrees+FMath::Cos(Cycle*2.f)*1.5f*Data.StrideBlend
        -Data.ExternalPullForward*PullAlpha*3.f;
    const float TargetRoll=-Data.LeanAmount*MaximumLeanDegrees-Data.SlipAmount*SlipLeanDegrees
        +Data.ExternalPullSide*PullAlpha*PullLeanDegrees;
    const float TargetYaw=Data.ExternalPullSide*PullAlpha*PullYawDegrees;
    Data.BodyBob=FMath::FInterpTo(Data.BodyBob,TargetBob,Dt,PoseResponse);
    Data.BodyPitch=FMath::FInterpTo(Data.BodyPitch,TargetPitch,Dt,PoseResponse);
    Data.BodyRoll=FMath::FInterpTo(Data.BodyRoll,TargetRoll,Dt,PoseResponse);
    Data.BodyYaw=FMath::FInterpTo(Data.BodyYaw,TargetYaw,Dt,PoseResponse);

    if (auto* Anim=Cast<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance()))
    {
        Anim->ApplyHorseData(Data,TemporaryIdleAnimation,TemporaryWalkAnimation,TemporaryGallopAnimation,
            TemporaryStopAnimation,TemporaryStruggleAnimation,TemporaryStruggleAlternateAnimation);
    }
    if (auto* Skeletal=Horse->GetMesh(); Skeletal && Skeletal->GetSkeletalMeshAsset())
    {
        // Visual mesh only: capsule, actor heading and movement simulation remain authoritative.
        Skeletal->SetRelativeLocationAndRotation(SkeletalBaseLocation+FVector(0,0,Data.BodyBob),
            SkeletalBaseRotation+FRotator(Data.BodyPitch,Data.BodyYaw,Data.BodyRoll));
    }

    if (auto* Mesh=Horse->GetPlaceholderRoot())
    {
        if (bAnimatePlaceholder)
        {
            Mesh->SetRelativeLocationAndRotation(BaseLocation+FVector(0,0,Data.BodyBob),
                BaseRotation+FRotator(Data.BodyPitch,Data.BodyYaw,Data.BodyRoll));
        }
        else
        {
            Mesh->SetRelativeLocationAndRotation(BaseLocation,BaseRotation);
        }
        Horse->AnimatePlaceholderLegs(Data.GaitPhase,bAnimatePlaceholder?Data.StrideBlend:0.f);
    }
}
