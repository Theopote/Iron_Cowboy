#include "Character/Horse/HorseAttributeComponent.h"
UHorseAttributeComponent::UHorseAttributeComponent() { PrimaryComponentTick.bCanEverTick = false; }
void UHorseAttributeComponent::BeginPlay() { Super::BeginPlay(); CurrentStamina = FMath::Max(0.f, MaxStamina); }
float UHorseAttributeComponent::GetStaminaNormalized() const { return FMath::Clamp(CurrentStamina / FMath::Max(1.f, MaxStamina), 0.f, 1.f); }
void UHorseAttributeComponent::UpdateStamina(float DeltaTime, float Multiplier)
{
    const float Rate = Multiplier > 0 ? -FMath::Max(0.f, StaminaDrainRate) : -FMath::Max(0.f, StaminaRecoveryRate);
    CurrentStamina = bInfiniteStamina ? FMath::Max(0.f, MaxStamina) : FMath::Clamp(CurrentStamina + Rate * Multiplier * FMath::Max(0.f, DeltaTime), 0.f, FMath::Max(0.f, MaxStamina));
}
