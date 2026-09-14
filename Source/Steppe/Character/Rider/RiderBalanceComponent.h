#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lasso/LassoComponent.h"
#include "RiderBalanceComponent.generated.h"

UENUM(BlueprintType)
enum class ERiderBalanceState : uint8 { Stable, Warning, Falling, Dragged, Pulled, Recovering };

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API URiderBalanceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URiderBalanceComponent();
    virtual void TickComponent(float DeltaSeconds,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;

    float CalculateLoad(float TensionValue,float LateralValue,float Speed,float TargetStrength,ELassoHitZone Zone) const;

    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="0")) float SafeSpeed = 300.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="1")) float CriticalSpeed = 1300.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="0")) float BuildThreshold = .15f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="0.01")) float BuildPerSecond = .9f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="0.01")) float RecoveryPerSecond = .45f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="0.01")) float WarningThreshold = .55f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Balance",meta=(ClampMin="0.01")) float FallThreshold = 1.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Fall",meta=(ClampMin="0")) float FallHorizontalSpeed = 420.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Fall",meta=(ClampMin="0")) float FallUpSpeed = 260.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Fall",meta=(ClampMin="0.1")) float FallSeconds = .35f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Dragged",meta=(ClampMin="0.1")) float MaximumDraggedSeconds = 2.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Dragged",meta=(ClampMin="100")) float MaximumDraggedDistance = 1800.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Dragged",meta=(ClampMin="0")) float DragSpeed = 520.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="On Foot",meta=(ClampMin="0")) float OnFootPullAcceleration = 1500.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="On Foot",meta=(ClampMin="0")) float MaximumOnFootPullSpeed = 720.f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="On Foot",meta=(ClampMin="0",ClampMax="1.5")) float OnFootPullThreshold = .15f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Recovery",meta=(ClampMin="0.1")) float PostFallRecoverySeconds = .8f;

    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Balance") ERiderBalanceState State = ERiderBalanceState::Stable;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Balance") float Balance = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Balance") float CurrentLoad = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Balance") float LateralPull = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Dragged") float DraggedRemaining = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Balance") FString Feedback = TEXT("Balance safe");

private:
    float StateRemaining = 0.f;
    void UpdateMountedBalance(float DeltaSeconds);
    void TriggerFall();
    void UpdateFall(float DeltaSeconds);
    void UpdateDragged(float DeltaSeconds);
    void UpdateOnFootRope(float DeltaSeconds);
    void BeginRecovery(const TCHAR* Message);
};
