#pragma once
#include "CoreMinimal.h"
#include "HorseMovementTypes.generated.h"

UENUM(BlueprintType)
enum class EHorseGait : uint8 { Idle, Walk, Trot, Canter, Gallop, Sprint };
UENUM(BlueprintType)
enum class EHorseMovementState : uint8 { Grounded, Stumbling, Falling, Disabled };

USTRUCT(BlueprintType)
struct STEPPE_API FHorseMovementIntent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DesiredSpeed = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DesiredTurn = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float BrakeStrength = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EHorseGait RequestedGait = EHorseGait::Idle;
};

USTRUCT(BlueprintType)
struct STEPPE_API FHorseAnimationData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Speed = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NormalizedSpeed = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EHorseGait Gait = EHorseGait::Idle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NormalizedAcceleration = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AccelerationAmount = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DecelerationAmount = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TurnAmount = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float LeanAmount = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SlipAmount = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool IsGrounded = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool IsStumbling = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StaminaNormalized = 1.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float GaitPhase = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StrideBlend = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BodyBob = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BodyPitch = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BodyRoll = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float FootContactPulse = 0.f;
};

namespace SteppeUnits
{
    inline float ToMetersPerSecond(float CmPerSecond) { return CmPerSecond / 100.f; }
    inline float ToKilometersPerHour(float CmPerSecond) { return ToMetersPerSecond(CmPerSecond) * 3.6f; }
}
