#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "AI/HorseBrainComponent.h"
ASteppeWildHorseCharacter::ASteppeWildHorseCharacter()
{
    bCanBeMounted = false;
    Brain = CreateDefaultSubobject<UHorseBrainComponent>(TEXT("HorseBrain"));
}
