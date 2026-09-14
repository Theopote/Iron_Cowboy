#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lasso/LassoComponent.h"
#include "Character/Rider/RiderBalanceComponent.h"
#include "Game/SteppeTrialState.h"
#include "SteppePlaytestMetrics.generated.h"

class ASteppeRiderCharacter;
class ASteppeHerdManager;

USTRUCT(BlueprintType)
struct STEPPE_API FSteppePlaytestRound
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString SessionId;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString StartedUtc;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Result = TEXT("Running");
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString EndReason;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString TargetHorse;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString TargetArchetype;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString HitZone;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ElapsedSeconds = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float TargetSelectedSeconds = -1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float IsolatedSeconds = -1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float CapturedSeconds = -1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float FirstContactSeconds = -1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float DeliveredSeconds = -1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float NamedSeconds = -1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Score = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 ThrowCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 AttachCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 MissCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 RopeBreakCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 ReleaseCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 DangerousTensionCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 BalanceWarningCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 FallCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 DraggedCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 RetryCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float PeakTension = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float PeakBalanceRisk = 0.f;

    void RecordLassoTransition(ELassoState Previous,ELassoState Current,const FString& Feedback,float Tension,ELassoHitZone Zone);
    void RecordBalanceTransition(ERiderBalanceState Previous,ERiderBalanceState Current,float Balance,float FallThreshold);
};

UCLASS(ClassGroup=(Steppe))
class STEPPE_API USteppePlaytestMetricsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    USteppePlaytestMetricsComponent();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void BeginRound(ASteppeRiderCharacter* InRider,ASteppeHerdManager* InHerd,const FSteppeTrialProgress& Trial);
    void Observe(const FSteppeTrialProgress& Trial);
    void FinalizeForRestart();
    UFUNCTION(BlueprintPure,Category="Playtest") FString GetCompactSummary() const;

    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Playtest") FSteppePlaytestRound Round;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Playtest") FString LastWrittenFile;
private:
    TWeakObjectPtr<ASteppeRiderCharacter> Rider;
    TWeakObjectPtr<ASteppeHerdManager> Herd;
    ELassoState PreviousLassoState = ELassoState::Stored;
    ERiderBalanceState PreviousBalanceState = ERiderBalanceState::Stable;
    bool bDangerousTension = false;
    bool bFinalized = false;
    void Finish(const TCHAR* Result,const TCHAR* Reason,const FSteppeTrialProgress* Trial);
    bool WriteJson();
};
