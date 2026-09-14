#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "HorsePresentationComponent.generated.h"

UCLASS(ClassGroup=(Steppe),meta=(BlueprintSpawnableComponent))
class STEPPE_API UHorsePresentationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHorsePresentationComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaSeconds,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;

    float GetCycleFrequency(EHorseGait Gait) const;

    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Presentation") bool bAnimatePlaceholder = true;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="0")) float MaximumBob = 10.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="0")) float MaximumLeanDegrees = 9.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="0")) float AccelerationPitchDegrees = 5.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="0")) float PoseResponse = 8.f;

private:
    FVector BaseLocation = FVector::ZeroVector;
    FRotator BaseRotation = FRotator::ZeroRotator;
    float PreviousPhase = 0.f;
};
