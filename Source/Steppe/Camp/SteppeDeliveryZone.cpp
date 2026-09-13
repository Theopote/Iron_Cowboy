#include "Camp/SteppeDeliveryZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASteppeDeliveryZone::ASteppeDeliveryZone()
{
    PrimaryActorTick.bCanEverTick=false;
    DeliveryBounds=CreateDefaultSubobject<UBoxComponent>(TEXT("DeliveryBounds"));
    SetRootComponent(DeliveryBounds);
    DeliveryBounds->InitBoxExtent(BoxExtent);
    DeliveryBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DeliveryBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    DeliveryBounds->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

    GroundMarker=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMarker"));
    BackRail=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackRail"));
    LeftRail=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRail"));
    RightRail=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRail"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (UStaticMeshComponent* Part : {GroundMarker.Get(),BackRail.Get(),LeftRail.Get(),RightRail.Get()})
    {
        Part->SetupAttachment(DeliveryBounds);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (Cube.Succeeded()) { Part->SetStaticMesh(Cube.Object); }
    }
    GroundMarker->SetRelativeLocation(FVector(0,0,-92));
    GroundMarker->SetRelativeScale3D(FVector(12.f,10.f,.08f));
    BackRail->SetRelativeLocation(FVector(-590,0,25));
    BackRail->SetRelativeScale3D(FVector(.18f,10.f,1.4f));
    LeftRail->SetRelativeLocation(FVector(0,-490,25));
    LeftRail->SetRelativeScale3D(FVector(12.f,.18f,1.4f));
    RightRail->SetRelativeLocation(FVector(0,490,25));
    RightRail->SetRelativeScale3D(FVector(12.f,.18f,1.4f));
}

bool ASteppeDeliveryZone::ContainsActor(const AActor* Actor) const
{
    if (!IsValid(Actor)) { return false; }
    const FVector Local=GetActorTransform().InverseTransformPosition(Actor->GetActorLocation());
    const FVector Extent=DeliveryBounds?DeliveryBounds->GetScaledBoxExtent():BoxExtent;
    return FMath::Abs(Local.X)<=Extent.X && FMath::Abs(Local.Y)<=Extent.Y && FMath::Abs(Local.Z)<=Extent.Z+150.f;
}
