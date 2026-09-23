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
    void ApplyRiderPose(UAnimSequence* Base, UAnimSequence* UpperBody, float PlayRate);
    UAnimSequence* GetActiveSequence() const { return ActiveUpperBodySequence?ActiveUpperBodySequence.Get():ActiveBaseSequence.Get(); }
    UAnimSequence* GetActiveBaseSequence() const { return ActiveBaseSequence.Get(); }
    UAnimSequence* GetActiveUpperBodySequence() const { return ActiveUpperBodySequence.Get(); }

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rider|Animation") float PosePlayRate = 1.f;

    // Mesh-component-space targets. Only the mounted pose uses these presentation constraints.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rider|Contact") float ContactAlpha = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rider|Contact") FVector LeftFootTarget = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rider|Contact") FVector RightFootTarget = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rider|Contact") FVector LeftReinTarget = FVector::ZeroVector;

private:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveBaseSequence;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActiveBaseMontage;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveUpperBodySequence;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActiveUpperBodyMontage;
};
