#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LassoTargetComponent.generated.h"

UENUM(BlueprintType)
enum class ELassoHitZone : uint8 { None, Head, Neck, Torso };

USTRUCT(BlueprintType)
struct FLassoTargetVolume
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lasso") FName Name = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lasso") ELassoHitZone Zone = ELassoHitZone::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lasso") FVector LocalCenter = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lasso", meta=(ClampMin="1")) float Radius = 10.f;
};

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API ULassoTargetComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULassoTargetComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lasso") TArray<FLassoTargetVolume> Volumes;

    FVector GetVolumeCenter(const FLassoTargetVolume& Volume) const;
    bool FindLoopIntersection(const FVector& PreviousCenter, const FVector& NextCenter,
        const FVector& PlaneNormal, float LoopRadius, float PlaneThickness,
        float& OutAlong, FVector& OutLocation, ELassoHitZone& OutZone) const;
    ELassoHitZone ClassifyLocation(const FVector& WorldLocation) const;
};
