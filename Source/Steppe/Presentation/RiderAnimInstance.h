#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RiderAnimInstance.generated.h"

class UAnimSequence;
class UAnimMontage;

// Presentation-only animation bridge. Gameplay selects the pose; this class owns blending.
UCLASS(Blueprintable, BlueprintType)
class STEPPE_API URiderAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    void ApplyRiderPose(UAnimSequence* Desired, float PlayRate);
    UAnimSequence* GetActiveSequence() const { return ActiveSequence; }

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rider|Animation") float PosePlayRate = 1.f;

private:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveSequence;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActiveMontage;
};
