#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SteppeGrassField.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

UCLASS()
class STEPPE_API ASteppeGrassField : public AActor
{
    GENERATED_BODY()
public:
    ASteppeGrassField();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Grass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Grass") int32 InstanceCount=3200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Grass") float HalfExtent=38000.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Grass") TObjectPtr<UMaterialInterface> GrassMaterial;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Grass") void RebuildInstances();
};
