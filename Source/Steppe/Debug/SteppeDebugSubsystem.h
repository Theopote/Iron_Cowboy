#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SteppeDebugSubsystem.generated.h"
UCLASS()
class STEPPE_API USteppeDebugSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure) bool IsHorseDebugEnabled() const;
    UFUNCTION(BlueprintPure) bool IsMovementDebugEnabled() const;
};
