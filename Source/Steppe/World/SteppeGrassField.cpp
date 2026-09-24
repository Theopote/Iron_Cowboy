#include "World/SteppeGrassField.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "LandscapeProxy.h"

ASteppeGrassField::ASteppeGrassField()
{
    PrimaryActorTick.bCanEverTick=false;
    Grass=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassInstances"));
    SetRootComponent(Grass);
    Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Grass->SetCastShadow(false);
    Grass->SetCullDistances(2500,22000);
    Grass->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
}

void ASteppeGrassField::RebuildInstances()
{
    Grass->ClearInstances();
    if (GrassMaterial) { Grass->SetMaterial(0,GrassMaterial); }
    FRandomStream Random(166);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SteppeGrassPlacement),false,this);
    for (int32 Index=0;Index<InstanceCount;++Index)
    {
        const float X=Random.FRandRange(-HalfExtent,HalfExtent);
        const float Y=Random.FRandRange(-HalfExtent,HalfExtent);
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit,FVector(X,Y,5000),FVector(X,Y,-5000),ECC_Visibility,Query)) { continue; }
        if (!Cast<ALandscapeProxy>(Hit.GetActor())) { continue; }
        const float Height=Random.FRandRange(28.f,58.f);
        const float Width=Random.FRandRange(2.5f,5.f);
        const FTransform Transform(FRotator(0,Random.FRandRange(0.f,180.f),0),Hit.ImpactPoint+FVector(0,0,Height*.5f),FVector(Width/100.f,Width/100.f,Height/100.f));
        Grass->AddInstance(Transform,true);
    }
    Grass->BuildTreeIfOutdated(true,true);
    MarkPackageDirty();
}
