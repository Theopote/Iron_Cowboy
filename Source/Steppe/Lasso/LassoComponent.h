#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LassoComponent.generated.h"

class ASteppeWildHorseCharacter;

UENUM(BlueprintType)
enum class ELassoState : uint8 { Stored, Aiming, Thrown, Attached, Recovering };

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API ULassoComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULassoComponent();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UFUNCTION(BlueprintCallable, Category="Lasso") bool BeginAim();
    bool BeginAimForTarget(ASteppeWildHorseCharacter* Target, bool bIsolated);
    UFUNCTION(BlueprintCallable, Category="Lasso") void CancelAim();
    UFUNCTION(BlueprintCallable, Category="Lasso") bool Throw();
    bool ThrowFrom(FVector Origin, FVector Direction);
    UFUNCTION(BlueprintCallable, Category="Lasso") void Release();
    UFUNCTION(BlueprintPure, Category="Lasso") FGameplayTag GetStateTag() const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="100")) float ThrowSpeed = 3200.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="100")) float MaximumRange = 2600.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="1")) float CaptureRadius = 80.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="0.1")) float RecoverySeconds = .75f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") ELassoState State = ELassoState::Stored;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") FVector RopeStart = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") FVector LoopLocation = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") float TravelDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") FString Feedback = TEXT("Select and isolate a target");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") TWeakObjectPtr<ASteppeWildHorseCharacter> Target;
private:
    FVector ThrowDirection = FVector::ForwardVector;
    float RecoveryRemaining = 0.f;
    void StartRecovery(const TCHAR* Message);
};
