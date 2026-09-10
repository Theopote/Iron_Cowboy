#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HorseAttributeComponent.generated.h"
UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API UHorseAttributeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHorseAttributeComponent();
    virtual void BeginPlay() override;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="1")) float MaxSpeed = 1500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="1")) float Acceleration = 350.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="1")) float Deceleration = 200.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="0.1")) float Agility = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="1")) float BaseTurnRate = 100.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="1")) float MaxStamina = 100.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float CurrentStamina = 100.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="0")) float StaminaDrainRate = 12.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse", meta=(ClampMin="0")) float StaminaRecoveryRate = 10.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse") float Strength = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse") float BodyMass = 500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug") bool bInfiniteStamina = false;
    UFUNCTION(BlueprintPure) float GetStaminaNormalized() const;
    void UpdateStamina(float DeltaTime, float Multiplier);
};
