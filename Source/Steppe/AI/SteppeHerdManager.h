#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/WildHorseArchetype.h"
#include "SteppeHerdManager.generated.h"

class ASteppeWildHorseCharacter;
class ASteppeRiderCharacter;
class ASteppeDeliveryZone;

UCLASS()
class STEPPE_API ASteppeHerdManager : public AActor
{
    GENERATED_BODY()
public:
    ASteppeHerdManager();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UFUNCTION(BlueprintCallable, Category="Herd") void SetThreatTarget(AActor* Target);
    UFUNCTION(BlueprintPure, Category="Herd") ASteppeWildHorseCharacter* FindFocusHorse(FVector ObserverLocation, FVector ViewDirection) const;
    UFUNCTION(BlueprintCallable, Category="Herd") ASteppeWildHorseCharacter* SelectFocusHorse(FVector ObserverLocation, FVector ViewDirection);
    UFUNCTION(BlueprintCallable, Category="Herd") void SetFocusedHorse(ASteppeWildHorseCharacter* Horse);
    UFUNCTION(BlueprintCallable, Category="Herd") void ClearFocusedHorse();
    UFUNCTION(BlueprintCallable, Category="Capture") bool RegisterCapturedHorse(ASteppeWildHorseCharacter* Horse);
    UFUNCTION(BlueprintCallable, Category="Capture") bool HandleFirstContactInteraction(ASteppeRiderCharacter* Rider);
    UFUNCTION(BlueprintCallable, Category="Capture") bool ConfirmDeliveredHorseName(AActor* HorseActor, const FString& NewName);
    void SetDeliveryZone(ASteppeDeliveryZone* Zone);
    void EnsureMembersSpawned();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="1", ClampMax="12")) int32 HerdSize = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="100")) float FormationSpacing = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="100")) float NeighborRadius = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="50")) float SeparationDistance = 550.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="1")) float AlarmPropagationSpeed = 900.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0.1")) float AlarmHoldSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0", ClampMax="1")) float AlarmStrength = .85f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd") int32 HerdSeed = 8347;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Isolation", meta=(ClampMin="100")) float FocusSelectionDistance = 6000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Isolation", meta=(ClampMin="-1", ClampMax="1")) float FocusSelectionMinDot = .4f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Isolation", meta=(ClampMin="100")) float IsolationDistanceRequired = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Isolation", meta=(ClampMin="0.1")) float IsolationHoldSeconds = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd") TSubclassOf<ASteppeWildHorseCharacter> HorseClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Archetype") TArray<FWildHorseArchetypeProfile> ArchetypeProfiles;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") TArray<TObjectPtr<ASteppeWildHorseCharacter>> Members;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") FVector HerdCenter = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") FVector AverageVelocity = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") int32 AlarmSourceCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") float MinimumMemberSpacing = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Isolation") TObjectPtr<ASteppeWildHorseCharacter> FocusedHorse;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Isolation") FVector RestHerdCenter = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Isolation") float IsolationDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Isolation") float IsolationProgress = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Isolation") bool bTargetIsolated = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Capture") TArray<TObjectPtr<ASteppeWildHorseCharacter>> CapturedHorses;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Capture") int32 CapturedCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Capture") TArray<TObjectPtr<ASteppeWildHorseCharacter>> FirstContactHorses;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Capture") int32 FirstContactCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Delivery") TObjectPtr<ASteppeWildHorseCharacter> LeadingHorse;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Delivery") TArray<TObjectPtr<ASteppeWildHorseCharacter>> DeliveredHorses;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Delivery") int32 DeliveredCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Delivery") int32 NamedCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Delivery") TObjectPtr<ASteppeDeliveryZone> DeliveryZone;

private:
    UPROPERTY() TWeakObjectPtr<AActor> ThreatTarget;
    TMap<TWeakObjectPtr<ASteppeWildHorseCharacter>, float> AlarmTravelSeconds;
    float IsolationSeconds = 0.f;
};
