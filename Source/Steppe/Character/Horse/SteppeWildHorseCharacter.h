#pragma once
#include "Character/Horse/SteppeHorseCharacter.h"
#include "SteppeWildHorseCharacter.generated.h"
class UHorseBrainComponent;
UCLASS()
class STEPPE_API ASteppeWildHorseCharacter : public ASteppeHorseCharacter
{
    GENERATED_BODY()
public:
    ASteppeWildHorseCharacter();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wild Horse") TObjectPtr<UHorseBrainComponent> Brain;
};
