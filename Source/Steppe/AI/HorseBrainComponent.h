#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "HorseBrainComponent.generated.h"
class UWildHorseConfig;
class ASteppeHorseCharacter;

UENUM(BlueprintType)
enum class EWildHorseState : uint8 { Roaming, Alert, Fleeing, Recovering, Yielding, Lassoed, Captured };

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API UHorseBrainComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHorseBrainComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    UFUNCTION(BlueprintCallable, Category="Wild Horse") void SetThreatTarget(AActor* Target);
    UFUNCTION(BlueprintCallable, Category="Wild Horse") void ReceiveHerdAlarm(float Strength, float Duration);
    UFUNCTION(BlueprintCallable, Category="Wild Horse") void SetLassoed(bool bNewLassoed);
    void SetLassoConstraint(FVector Anchor, float Tension, bool bBraced);
    UFUNCTION(BlueprintCallable, Category="Wild Horse") void SetCaptured(bool bNewCaptured);
    void RequestCapturedRetreat(FVector Direction, float Speed, float Duration);
    void SetLeadTarget(AActor* Target);
    void SetHerdGuidance(FVector Center, FVector Velocity, FVector Separation, int32 NeighborCount);
    void SetHerdIdentity(int32 MemberIndex, int32 HerdSeed);
    UFUNCTION(BlueprintPure, Category="Wild Horse") FGameplayTag GetBehaviorTag() const;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wild Horse") TObjectPtr<UWildHorseConfig> Config;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") EWildHorseState State = EWildHorseState::Roaming;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float Awareness = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float ThreatDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float ClosingSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bThreatVisible = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bPathBlocked = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bBrakingForHazard = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float StoppingProbeDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") FVector Goal = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") FVector SteeringDirection = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float ApproachSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float HerdAlarmSeconds = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") int32 HerdNeighborCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") FVector HerdSeparation = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") FVector DynamicAvoidance = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") int32 HerdMemberIndex = INDEX_NONE;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float IndividualReactionScale = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float IndividualSteeringBias = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bRecoveringFromBlockage = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bIsolationFocus = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bLassoed = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") float LassoTension = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Wild Horse") bool bCaptured = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") float AwarenessRiseScale = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") float AwarenessDecayScale = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") float FlightSpeedScale = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Archetype") float StruggleSpeedScale = 1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lead") bool bLeading = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lead") float LeadDistance = 0.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lead", meta=(ClampMin="50")) float LeadFollowDistance = 280.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lead", meta=(ClampMin="50")) float LeadMoveThreshold = 380.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lead", meta=(ClampMin="50")) float LeadMaxDistance = 1800.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lead", meta=(ClampMin="10")) float LeadWalkSpeed = 300.f;
    const UWildHorseConfig& GetConfig() const;
private:
    void Sense(float Dt, const ASteppeHorseCharacter& Horse);
    void ChangeState(EWildHorseState NewState);
    FVector FindSafeDirection(const ASteppeHorseCharacter& Horse, FVector Desired);
    bool IsDirectionSupported(const ASteppeHorseCharacter& Horse, FVector Direction, float Distance) const;
    void ChooseRoamGoal();
    UPROPERTY() TWeakObjectPtr<AActor> ThreatTarget;
    FVector Home = FVector::ZeroVector;
    FVector LastThreatPosition = FVector::ZeroVector;
    FRandomStream Random;
    float StateSeconds = 0.f;
    float UnseenSeconds = 1000.f;
    float ReleasedSeconds = 0.f;
    float PauseRemaining = 0.f;
    float RoamGoalSeconds = 0.f;
    float RecoveryTurnRemaining = 0.f;
    float IndividualPauseScale = 1.f;
    int32 IdentitySeed = 0;
    float HerdAlarmStrength = 0.f;
    FVector HerdCenter = FVector::ZeroVector;
    FVector HerdVelocity = FVector::ZeroVector;
    FVector LassoAnchor = FVector::ZeroVector;
    bool bLassoBraced = false;
    FVector CapturedRetreatDirection = FVector::ZeroVector;
    float CapturedRetreatSpeed = 0.f;
    float CapturedRetreatSeconds = 0.f;
    UPROPERTY() TWeakObjectPtr<AActor> LeadTarget;
};
