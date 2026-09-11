#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "HorseBrainComponent.generated.h"
class UWildHorseConfig;
class ASteppeHorseCharacter;

UENUM(BlueprintType)
enum class EWildHorseState : uint8 { Roaming, Alert, Fleeing, Recovering };

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API UHorseBrainComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHorseBrainComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    UFUNCTION(BlueprintCallable, Category="Wild Horse") void SetThreatTarget(AActor* Target);
    UFUNCTION(BlueprintPure, Category="Wild Horse") FGameplayTag GetBehaviorTag() const;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wild Horse") TObjectPtr<UWildHorseConfig> Config;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") EWildHorseState State = EWildHorseState::Roaming;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float Awareness = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float ThreatDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float ClosingSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bThreatVisible = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bPathBlocked = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") FVector Goal = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") FVector SteeringDirection = FVector::ZeroVector;
    const UWildHorseConfig& GetConfig() const;
private:
    void Sense(float Dt, const ASteppeHorseCharacter& Horse);
    void ChangeState(EWildHorseState NewState);
    FVector FindSafeDirection(const ASteppeHorseCharacter& Horse, FVector Desired);
    void ChooseRoamGoal();
    UPROPERTY() TWeakObjectPtr<AActor> ThreatTarget;
    FVector Home = FVector::ZeroVector;
    FVector LastThreatPosition = FVector::ZeroVector;
    FRandomStream Random;
    float StateSeconds = 0.f;
    float UnseenSeconds = 1000.f;
    float PauseRemaining = 0.f;
    float RoamGoalSeconds = 0.f;
};
