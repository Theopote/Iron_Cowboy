#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "AI/HorseBrainComponent.h"
#include "Capture/HorseTrustComponent.h"
ASteppeWildHorseCharacter::ASteppeWildHorseCharacter()
{
    bCanBeMounted = false;
    Brain = CreateDefaultSubobject<UHorseBrainComponent>(TEXT("HorseBrain"));
    Trust = CreateDefaultSubobject<UHorseTrustComponent>(TEXT("HorseTrust"));
}
