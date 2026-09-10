#pragma once
#include "CoreMinimal.h"
#include "RidingIntent.generated.h"
USTRUCT(BlueprintType)
struct STEPPE_API FRidingIntent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Forward = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Turn = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSprint = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBrake = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D LookInput = FVector2D::ZeroVector;
    void Reset() { *this = FRidingIntent(); }
    void Clamp()
    {
        Forward = FMath::IsFinite(Forward) ? FMath::Clamp(Forward, -1.f, 1.f) : 0.f;
        Turn = FMath::IsFinite(Turn) ? FMath::Clamp(Turn, -1.f, 1.f) : 0.f;
        if (LookInput.ContainsNaN()) { LookInput = FVector2D::ZeroVector; }
    }
    bool IsNearlyZero() const { return FMath::IsNearlyZero(Forward) && FMath::IsNearlyZero(Turn) && !bSprint && !bBrake && LookInput.IsNearlyZero(); }
};
