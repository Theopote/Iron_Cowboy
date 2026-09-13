#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "AI/HorseBrainComponent.h"
#include "Capture/HorseTrustComponent.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
ASteppeWildHorseCharacter::ASteppeWildHorseCharacter()
{
    bCanBeMounted = false;
    Brain = CreateDefaultSubobject<UHorseBrainComponent>(TEXT("HorseBrain"));
    Trust = CreateDefaultSubobject<UHorseTrustComponent>(TEXT("HorseTrust"));
}

void ASteppeWildHorseCharacter::ApplyArchetype(const FWildHorseArchetypeProfile& Profile)
{
    if (bArchetypeApplied || !Attributes || !Brain || !Trust) { return; }
    bArchetypeApplied=true;
    Archetype=Profile.Archetype;
    ArchetypeLabel=Profile.DisplayName;
    const float StaminaRatio=Attributes->GetStaminaNormalized();
    Attributes->MaxSpeed*=FMath::Max(.1f,Profile.MaxSpeedMultiplier);
    Attributes->Acceleration*=FMath::Max(.1f,Profile.AccelerationMultiplier);
    Attributes->MaxStamina*=FMath::Max(.1f,Profile.StaminaMultiplier);
    Attributes->CurrentStamina=Attributes->MaxStamina*StaminaRatio;
    Attributes->Strength*=FMath::Max(.1f,Profile.StrengthMultiplier);
    Attributes->Agility*=FMath::Max(.1f,Profile.AgilityMultiplier);
    Brain->AwarenessRiseScale=FMath::Max(.1f,Profile.FearRiseMultiplier);
    Brain->AwarenessDecayScale=FMath::Max(.1f,Profile.FearDecayMultiplier);
    Brain->FlightSpeedScale=FMath::Max(.1f,Profile.FlightSpeedMultiplier);
    Brain->StruggleSpeedScale=FMath::Max(.1f,Profile.StruggleMultiplier);
    SubdueResistance=FMath::Max(.1f,Profile.SubdueResistanceMultiplier);
    Trust->SafeApproachSpeed*=FMath::Max(.1f,Profile.SafeApproachMultiplier);
    Trust->CalmHoldSeconds*=FMath::Max(.1f,Profile.CalmHoldMultiplier);
    Trust->Temperament=Profile.Temperament;
    Trust->Coat=Profile.Coat;
    if (auto* PlaceholderMesh=GetPlaceholderMesh())
    {
        if (auto* Material=PlaceholderMesh->CreateAndSetMaterialInstanceDynamic(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"),Profile.DebugColor);
        }
    }
}
