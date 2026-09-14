#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LassoComponent.generated.h"

class ASteppeWildHorseCharacter;
class ASteppeHerdManager;

UENUM(BlueprintType)
enum class ELassoState : uint8 { Stored, Aiming, Thrown, Attached, Recovering, Subdued, Captured };

UENUM(BlueprintType)
enum class ELassoHitZone : uint8 { None, Head, Neck, Torso };

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API ULassoComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULassoComponent();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UFUNCTION(BlueprintCallable, Category="Lasso") bool BeginAim();
    bool BeginAimForTarget(ASteppeWildHorseCharacter* Target, bool bIsolated);
    UFUNCTION(BlueprintCallable, Category="Lasso") void CancelAim();
    UFUNCTION(BlueprintCallable, Category="Lasso") bool Throw();
    bool ThrowFrom(FVector Origin, FVector Direction);
    UFUNCTION(BlueprintCallable, Category="Lasso") void Release();
    UFUNCTION(BlueprintCallable, Category="Lasso") void SetBracing(bool bNewBracing) { bBracing=bNewBracing; }
    UFUNCTION(BlueprintCallable, Category="Capture") bool Capture();
    bool CaptureWithHerd(ASteppeHerdManager* Herd);
    bool CompleteOnFootSurrender(ASteppeHerdManager* Herd);
    UFUNCTION(BlueprintPure, Category="Lasso") FGameplayTag GetStateTag() const;
    float GetEffectiveSubdueSeconds(const ASteppeWildHorseCharacter* Horse) const;
    ELassoHitZone ClassifyHitZone(const ASteppeWildHorseCharacter* Horse, FVector HitLocation) const;
    float GetHitZoneTensionMultiplier() const;
    float CalculateShockLoad(float SeparatingSpeed, float AnchorDeceleration, float TensionValue) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="100")) float ThrowSpeed = 3200.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="100")) float MaximumRange = 2600.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="1")) float CaptureRadius = 80.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso", meta=(ClampMin="0.1")) float RecoverySeconds = 1.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso|Swing", meta=(ClampMin="0.1")) float ReadySeconds = .35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso|Swing", meta=(ClampMin="0.2")) float SwingPeriod = 1.2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso|Swing", meta=(ClampMin="0.1", ClampMax="1")) float UnstableRadiusMultiplier = .45f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso|Swing", meta=(ClampMin="0.1", ClampMax="1")) float UnstableSpeedMultiplier = .75f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lasso|Swing", meta=(ClampMin="0.1", ClampMax="1")) float UnstableRangeMultiplier = .8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight", meta=(ClampMin="0.1")) float SubdueSeconds = 3.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|On Foot", meta=(ClampMin="50")) float OnFootSurrenderDistance = 300.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|On Foot", meta=(ClampMin="0.1")) float OnFootSurrenderSeconds = 2.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|On Foot", meta=(ClampMin="0")) float OnFootSurrenderMaxRelativeSpeed = 260.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight", meta=(ClampMin="10")) float TensionRange = 500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight", meta=(ClampMin="0", ClampMax="1")) float UsefulTensionMin = .2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight", meta=(ClampMin="0", ClampMax="1.5")) float UsefulTensionMax = .85f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Shock", meta=(ClampMin="1")) float ShockSpeedThreshold = 900.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Shock", meta=(ClampMin="1")) float ShockDecelerationThreshold = 700.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Shock", meta=(ClampMin="0.1")) float ShockBreakThreshold = 1.35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Shock", meta=(ClampMin="0.1")) float BreakHoldSeconds = .8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Shock", meta=(ClampMin="0.1")) float ShockDecayPerSecond = .75f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Shock", meta=(ClampMin="1.1")) float EmergencyBreakRangeMultiplier = 1.8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Obstacle", meta=(ClampMin="0",ClampMax="1")) float ObstacleWrapTensionBonus = .25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope Fight|Obstacle", meta=(ClampMin="0.05")) float RopeWrapClearSeconds = .35f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") ELassoState State = ELassoState::Stored;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") FVector RopeStart = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") FVector LoopLocation = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") float TravelDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float AimSeconds = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float SwingPhase = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float SwingStability = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float LastThrowStability = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float EffectiveCaptureRadius = 80.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float EffectiveThrowSpeed = 3200.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Swing") float EffectiveMaximumRange = 2600.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso|Hit Zone") ELassoHitZone HitZone = ELassoHitZone::None;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") FString Feedback = TEXT("Select and isolate a target");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") TWeakObjectPtr<ASteppeWildHorseCharacter> Target;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lasso") bool bTargetIsolated = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight") float RopeLength = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight") float Tension = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight") float ControlProgress = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|Obstacle") bool bRopeWrapped = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|Obstacle") FVector RopeBendPoint = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|On Foot") float OnFootSurrenderProgress = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight") bool bBracing = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|Shock") float SeparatingSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|Shock") float AnchorDeceleration = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|Shock") float ShockLoad = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rope Fight|Shock") bool bShockRisk = false;
private:
    FVector ThrowDirection = FVector::ForwardVector;
    float RecoveryRemaining = 0.f;
    float ShockRiskSeconds = 0.f;
    float PreviousAnchorSpeed = 0.f;
    bool bHadAnchorSample = false;
    float RopeWrapClearTime = 0.f;
    void StartRecovery(const TCHAR* Message);
    void UpdateSwing(float DeltaSeconds);
    void UpdateRopeObstacle(float DeltaSeconds);
};
