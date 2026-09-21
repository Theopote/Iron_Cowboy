#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "HorseAnimInstance.generated.h"

class UAnimSequence;
class UAnimMontage;

// Animation owns transitions; horse movement remains the sole source of speed and gait.
UCLASS(Blueprintable, BlueprintType)
class STEPPE_API UHorseAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    void ApplyHorseData(const FHorseAnimationData& Data, UAnimSequence* Idle,
        UAnimSequence* Walk, UAnimSequence* Gallop, UAnimSequence* Stop, UAnimSequence* Struggle);
    UAnimSequence* GetActiveSequence() const { return ActiveSequence; }

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Animation") FHorseAnimationData HorseData;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Animation") float LocomotionPlayRate = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Animation") EHorseGait ActiveGait = EHorseGait::Idle;

private:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveSequence;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActiveMontage;
};
