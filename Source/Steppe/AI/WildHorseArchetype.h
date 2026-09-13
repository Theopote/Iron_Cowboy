#pragma once

#include "CoreMinimal.h"
#include "WildHorseArchetype.generated.h"

UENUM(BlueprintType)
enum class EWildHorseArchetype : uint8 { Fast, Strong, Nervous };

USTRUCT(BlueprintType)
struct STEPPE_API FWildHorseArchetypeProfile
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EWildHorseArchetype Archetype = EWildHorseArchetype::Fast;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName = TEXT("Fast");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Temperament = TEXT("Keen");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Coat = TEXT("Dun");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor DebugColor = FLinearColor(.65f,.42f,.16f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float MaxSpeedMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float AccelerationMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float StaminaMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float StrengthMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float AgilityMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float FearRiseMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float FearDecayMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float FlightSpeedMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float StruggleMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float SubdueResistanceMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float SafeApproachMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float CalmHoldMultiplier = 1.f;
};
