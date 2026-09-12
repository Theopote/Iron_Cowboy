#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SteppeHerdManager.generated.h"

class ASteppeWildHorseCharacter;

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
    void EnsureMembersSpawned();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="1", ClampMax="12")) int32 HerdSize = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="100")) float FormationSpacing = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="100")) float NeighborRadius = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="50")) float SeparationDistance = 550.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="1")) float AlarmPropagationSpeed = 900.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0.1")) float AlarmHoldSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0", ClampMax="1")) float AlarmStrength = .85f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd") int32 HerdSeed = 8347;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Herd") TSubclassOf<ASteppeWildHorseCharacter> HorseClass;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") TArray<TObjectPtr<ASteppeWildHorseCharacter>> Members;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") FVector HerdCenter = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") FVector AverageVelocity = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") int32 AlarmSourceCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Herd") float MinimumMemberSpacing = 0.f;

private:
    UPROPERTY() TWeakObjectPtr<AActor> ThreatTarget;
    TMap<TWeakObjectPtr<ASteppeWildHorseCharacter>, float> AlarmTravelSeconds;
};
