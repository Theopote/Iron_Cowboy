#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SteppeTerrainZone.generated.h"
class UBoxComponent;
UENUM(BlueprintType) enum class ESteppeTerrainZoneType : uint8 { ShallowWater, HardGround };
UCLASS()
class STEPPE_API ASteppeTerrainZone : public AActor
{
    GENERATED_BODY()
public:
    ASteppeTerrainZone();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UBoxComponent> Volume;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Terrain") ESteppeTerrainZoneType ZoneType=ESteppeTerrainZoneType::ShallowWater;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Terrain",meta=(ClampMin="0.1",ClampMax="1")) float MovementScale=.62f;
    UFUNCTION() void Enter(UPrimitiveComponent* Overlapped,AActor* Actor,UPrimitiveComponent* Other,int32 BodyIndex,bool bFromSweep,const FHitResult& Hit);
    UFUNCTION() void Leave(UPrimitiveComponent* Overlapped,AActor* Actor,UPrimitiveComponent* Other,int32 BodyIndex);
};
