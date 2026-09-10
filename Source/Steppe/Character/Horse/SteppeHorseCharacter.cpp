#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Core/SteppeGameplayTags.h"
ASteppeHorseCharacter::ASteppeHorseCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UHorseMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    GetCapsuleComponent()->InitCapsuleSize(55,95);
    Attributes = CreateDefaultSubobject<UHorseAttributeComponent>(TEXT("HorseAttributes"));
    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    Placeholder->SetupAttachment(GetRootComponent());
    Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Shape.Succeeded()) { Placeholder->SetStaticMesh(Shape.Object); }
    Placeholder->SetRelativeScale3D(FVector(1.8f,.65f,1.f));
    bUseControllerRotationYaw = false;
}
const UHorseLocomotionConfig* ASteppeHorseCharacter::GetLocomotionConfig() const
{
    return LocomotionConfig ? LocomotionConfig.Get() : GetDefault<UHorseLocomotionConfig>();
}
FGameplayTag ASteppeHorseCharacter::GetGaitTag() const
{
    switch (AnimationData.Gait)
    {
    case EHorseGait::Walk: return SteppeTags::Horse_Gait_Walk;
    case EHorseGait::Trot: return SteppeTags::Horse_Gait_Trot;
    case EHorseGait::Canter: return SteppeTags::Horse_Gait_Canter;
    case EHorseGait::Gallop: return SteppeTags::Horse_Gait_Gallop;
    case EHorseGait::Sprint: return SteppeTags::Horse_Gait_Sprint;
    default: return SteppeTags::Horse_Gait_Idle;
    }
}
FGameplayTag ASteppeHorseCharacter::GetStateTag() const
{
    if (AnimationData.IsStumbling) { return SteppeTags::Horse_State_Stumbling; }
    if (!AnimationData.IsGrounded) { return SteppeTags::Horse_State_Falling; }
    return AnimationData.Gait==EHorseGait::Idle?SteppeTags::Horse_State_Idle:SteppeTags::Horse_State_Moving;
}
