#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SteppeGameMode.generated.h"
class ASteppeHorseCharacter;
UCLASS()
class STEPPE_API ASteppeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASteppeGameMode();
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    UPROPERTY(EditDefaultsOnly, Category="Playground") bool bStartMounted = true;
    UPROPERTY(EditDefaultsOnly, Category="Playground") bool bDebugEnabled = true;
    UPROPERTY(EditDefaultsOnly, Category="Playground") bool bInfiniteStamina = false;
    UPROPERTY(EditDefaultsOnly, Category="Playground") FTransform HorseSpawnTransform = FTransform(FVector(200,0,110));
    UPROPERTY(EditDefaultsOnly, Category="Playground") TSubclassOf<ASteppeHorseCharacter> HorseClass;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Playground") TObjectPtr<ASteppeHorseCharacter> PlaygroundHorse;
};
