#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Core/SteppeGameplayTags.h"
#include "Presentation/HorsePresentationComponent.h"
ASteppeHorseCharacter::ASteppeHorseCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UHorseMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    GetCapsuleComponent()->InitCapsuleSize(55,95);
    Attributes = CreateDefaultSubobject<UHorseAttributeComponent>(TEXT("HorseAttributes"));
    Presentation = CreateDefaultSubobject<UHorsePresentationComponent>(TEXT("HorsePresentation"));
    Presentation->AddTickPrerequisiteComponent(GetCharacterMovement());
    PlaceholderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlaceholderHorseRoot"));
    PlaceholderRoot->SetupAttachment(GetRootComponent());
    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    Placeholder->SetupAttachment(PlaceholderRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    auto Configure=[this](UStaticMeshComponent* Part,UStaticMesh* ShapeMesh,FVector Location,FVector Scale,FRotator Rotation=FRotator::ZeroRotator)
    {
        Part->SetupAttachment(PlaceholderRoot);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetStaticMesh(ShapeMesh);
        Part->SetRelativeLocation(Location);
        Part->SetRelativeRotation(Rotation);
        Part->SetRelativeScale3D(Scale);
        PlaceholderParts.Add(Part);
    };
    Configure(Placeholder,Cube.Object,FVector(0,0,12),FVector(1.5f,.55f,.62f));
    auto AddPart=[this,&Configure](const TCHAR* Name,UStaticMesh* ShapeMesh,FVector Location,FVector Scale,FRotator Rotation=FRotator::ZeroRotator)
    {
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
        Configure(Part,ShapeMesh,Location,Scale,Rotation);
        return Part;
    };
    AddPart(TEXT("PlaceholderChest"),Sphere.Object,FVector(53,0,20),FVector(.65f,.5f,.72f));
    AddPart(TEXT("PlaceholderNeck"),Cylinder.Object,FVector(62,0,70),FVector(.29f,.29f,.78f),FRotator(22,0,0));
    AddPart(TEXT("PlaceholderHead"),Sphere.Object,FVector(91,0,116),FVector(.52f,.34f,.38f),FRotator(0,0,0));
    AddPart(TEXT("PlaceholderMuzzle"),Cube.Object,FVector(126,0,105),FVector(.42f,.3f,.24f),FRotator(-5,0,0));
    AddPart(TEXT("PlaceholderEarL"),Cone.Object,FVector(78,-15,143),FVector(.11f,.11f,.25f));
    AddPart(TEXT("PlaceholderEarR"),Cone.Object,FVector(78,15,143),FVector(.11f,.11f,.25f));
    AddPart(TEXT("PlaceholderTail"),Cylinder.Object,FVector(-91,0,24),FVector(.09f,.09f,.68f),FRotator(-38,0,0));
    const FVector LegPivots[]={FVector(50,-23,-17),FVector(50,23,-17),FVector(-49,-23,-17),FVector(-49,23,-17)};
    const TCHAR* PivotNames[]={TEXT("PlaceholderLegPivotFL"),TEXT("PlaceholderLegPivotFR"),TEXT("PlaceholderLegPivotRL"),TEXT("PlaceholderLegPivotRR")};
    const TCHAR* LegNames[]={TEXT("PlaceholderLegFL"),TEXT("PlaceholderLegFR"),TEXT("PlaceholderLegRL"),TEXT("PlaceholderLegRR")};
    const TCHAR* HoofNames[]={TEXT("PlaceholderHoofFL"),TEXT("PlaceholderHoofFR"),TEXT("PlaceholderHoofRL"),TEXT("PlaceholderHoofRR")};
    for (int32 Index=0; Index<4; ++Index)
    {
        auto* Pivot=CreateDefaultSubobject<USceneComponent>(FName(PivotNames[Index]));
        Pivot->SetupAttachment(PlaceholderRoot);
        Pivot->SetRelativeLocation(LegPivots[Index]);
        PlaceholderLegPivots.Add(Pivot);
        auto* Leg=AddPart(LegNames[Index],Cylinder.Object,FVector::ZeroVector,FVector(.13f,.13f,.72f));
        Leg->SetupAttachment(Pivot);
        Leg->SetRelativeLocation(FVector(0,0,-36));
        PlaceholderLegs.Add(Leg);
        auto* Hoof=AddPart(HoofNames[Index],Cube.Object,FVector::ZeroVector,FVector(.28f,.19f,.14f));
        Hoof->SetupAttachment(Pivot);
        Hoof->SetRelativeLocation(FVector(7,0,-78));
    }
    bUseControllerRotationYaw = false;
}

void ASteppeHorseCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (!Placeholder) { return; }
    UMaterialInterface* BodyMaterial=Placeholder->GetMaterial(0);
    for (UStaticMeshComponent* Part : PlaceholderParts)
    {
        if (Part && Part!=Placeholder) { Part->SetMaterial(0,BodyMaterial); }
    }
}

void ASteppeHorseCharacter::ApplyPlaceholderColor(const FLinearColor& Color)
{
    for (UStaticMeshComponent* Part : PlaceholderParts)
    {
        if (!Part) { continue; }
        if (auto* Material=Part->CreateAndSetMaterialInstanceDynamic(0)) { Material->SetVectorParameterValue(TEXT("Color"),Color); }
    }
}

void ASteppeHorseCharacter::AnimatePlaceholderLegs(float GaitPhase,float StrideBlend)
{
    const float Swing=FMath::Sin(GaitPhase*2.f*PI)*28.f*StrideBlend;
    for (int32 Index=0; Index<PlaceholderLegPivots.Num(); ++Index)
    {
        if (PlaceholderLegPivots[Index]) { PlaceholderLegPivots[Index]->SetRelativeRotation(FRotator((Index==0 || Index==3)?Swing:-Swing,0,0)); }
    }
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
