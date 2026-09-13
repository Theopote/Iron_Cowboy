#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SteppeDeliveryZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class STEPPE_API ASteppeDeliveryZone : public AActor
{
    GENERATED_BODY()
public:
    ASteppeDeliveryZone();
    bool ContainsActor(const AActor* Actor) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camp") TObjectPtr<UBoxComponent> DeliveryBounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camp") TObjectPtr<UStaticMeshComponent> GroundMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camp") TObjectPtr<UStaticMeshComponent> BackRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camp") TObjectPtr<UStaticMeshComponent> LeftRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camp") TObjectPtr<UStaticMeshComponent> RightRail;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camp") FVector BoxExtent = FVector(600.f,500.f,180.f);
};
