#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "HorseLocomotionConfig.generated.h"

USTRUCT(BlueprintType)
struct STEPPE_API FHorseGaitSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float TargetSpeed = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Acceleration = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Deceleration = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float TurnMultiplier = 1.f;
    // Positive drains stamina, negative recovers it, multiplied by the attribute rate.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float StaminaMultiplier = -1.f;
    FHorseGaitSettings() = default;
    FHorseGaitSettings(float S, float A, float D, float T, float E) : TargetSpeed(S), Acceleration(A), Deceleration(D), TurnMultiplier(T), StaminaMultiplier(E) {}
};

UCLASS(BlueprintType)
class STEPPE_API UHorseLocomotionConfig : public UDataAsset
{
    GENERATED_BODY()
public:
    UHorseLocomotionConfig();
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gaits") TArray<FHorseGaitSettings> Gaits;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turning") FRuntimeFloatCurve SpeedTurnCurve;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turning") float HeadingLookAhead = 35.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turning") float TurnDifficulty = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turning") float StressWarning = .55f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turning") float StressCritical = .85f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Response", meta=(ClampMin="0.01")) float ResponseSeconds = .25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Braking", meta=(ClampMin="1")) float EmergencyBrakeRate = 600.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gaits") float GaitHysteresis = 20.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gaits") float IdleThreshold = 5.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina") float ExhaustionThreshold = 5.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stamina") float SprintResumeThreshold = 25.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") FRuntimeFloatCurve SpeedFOVCurve;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") FRuntimeFloatCurve SpeedDistanceCurve;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float CameraBlendRate = 4.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float CameraLagSpeed = 8.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float AccelerationDistance = 20.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera") float TurnCameraOffset = 15.f;
    const FHorseGaitSettings& GetGait(EHorseGait Gait) const;
};
