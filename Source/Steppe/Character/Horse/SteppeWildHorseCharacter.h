#pragma once
#include "Character/Horse/SteppeHorseCharacter.h"
#include "AI/WildHorseArchetype.h"
#include "SteppeWildHorseCharacter.generated.h"
class UHorseBrainComponent;
class UHorseTrustComponent;
class ULassoTargetComponent;
UCLASS()
class STEPPE_API ASteppeWildHorseCharacter : public ASteppeHorseCharacter
{
    GENERATED_BODY()
public:
    ASteppeWildHorseCharacter();
    void ApplyArchetype(const FWildHorseArchetypeProfile& Profile);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wild Horse") TObjectPtr<UHorseBrainComponent> Brain;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture") TObjectPtr<UHorseTrustComponent> Trust;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lasso") TObjectPtr<ULassoTargetComponent> LassoTarget;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") EWildHorseArchetype Archetype = EWildHorseArchetype::Fast;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") FString ArchetypeLabel = TEXT("Fast");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") float SubdueResistance = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") bool bArchetypeApplied = false;
};
