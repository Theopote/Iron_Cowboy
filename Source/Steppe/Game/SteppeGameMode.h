#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/SteppeTrialState.h"
#include "SteppeGameMode.generated.h"
class ASteppeHorseCharacter;
class ASteppeWildHorseCharacter;
class ASteppeHerdManager;
class ASteppeDeliveryZone;
class USteppePlaytestMetricsComponent;
UCLASS()
class STEPPE_API ASteppeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASteppeGameMode();
    virtual void Tick(float DeltaSeconds) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    UPROPERTY(EditDefaultsOnly, Category="Playground") bool bStartMounted = true;
    UPROPERTY(EditDefaultsOnly, Category="Playground") bool bDebugEnabled = false;
    UPROPERTY(EditDefaultsOnly, Category="Playground") bool bInfiniteStamina = false;
    UPROPERTY(EditDefaultsOnly, Category="Playground") FTransform HorseSpawnTransform = FTransform(FVector(200,0,110));
    UPROPERTY(EditDefaultsOnly, Category="Playground") TSubclassOf<ASteppeHorseCharacter> HorseClass;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Playground") TObjectPtr<ASteppeHorseCharacter> PlaygroundHorse;
    UPROPERTY(EditDefaultsOnly, Category="Wild Horse") bool bSpawnWildHorse = true;
    UPROPERTY(EditDefaultsOnly, Category="Wild Horse") FTransform WildHorseSpawnTransform = FTransform(FVector(4000,0,110));
    UPROPERTY(EditDefaultsOnly, Category="Wild Horse") TSubclassOf<ASteppeWildHorseCharacter> WildHorseClass;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") TObjectPtr<ASteppeWildHorseCharacter> WildHorse;
    UPROPERTY(EditDefaultsOnly, Category="Wild Horse", meta=(ClampMin="1", ClampMax="15")) int32 WildHorseCount = 12;
    UPROPERTY(EditDefaultsOnly, Category="Wild Horse|Habitat", meta=(ClampMin="0")) FVector2D HerdHabitatExtents = FVector2D::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") TObjectPtr<ASteppeHerdManager> HerdManager;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") TArray<TObjectPtr<ASteppeWildHorseCharacter>> WildHorses;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camp") FTransform DeliveryZoneTransform = FTransform(FVector(-300,-900,100));
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camp") TSubclassOf<ASteppeDeliveryZone> DeliveryZoneClass;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Camp") TObjectPtr<ASteppeDeliveryZone> DeliveryZone;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Trial") bool bEnableTrial = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Trial", meta=(ClampMin="10")) float TrialDurationSeconds = 120.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Trial", meta=(ClampMin="1")) int32 RequiredCaptures = 1;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Trial") FSteppeTrialProgress Trial;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Playtest") TObjectPtr<USteppePlaytestMetricsComponent> PlaytestMetrics;
};
