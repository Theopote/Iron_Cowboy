#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Character/Rider/RidingIntent.h"
#include "GameplayTagContainer.h"
#include "SteppeRiderCharacter.generated.h"
class URidingComponent;
class URidingCameraComponent;
class USpringArmComponent;
class UCameraComponent;
class USteppeInputConfig;
class ULassoComponent;
class URiderBalanceComponent;
class USteppeFeedbackComponent;
class UAnimSequence;
struct FInputActionValue;

USTRUCT(BlueprintType)
struct STEPPE_API FRiderPresentationData
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bMounted = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bBracing = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bFalling = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bDragged = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BalanceRisk = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float PullSide = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float RopeTension = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float RopeYaw = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BodyYaw = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BodyPitch = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BodyRoll = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float SeatOffsetZ = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Speed = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float SwingPhase = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float SwingStability = 0.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bAimingLasso = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bLassoThrown = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bRopeAttached = false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bLeadingHorse = false;
};

UCLASS()
class STEPPE_API ASteppeRiderCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ASteppeRiderCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void PawnClientRestart() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URidingComponent> Riding;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URidingCameraComponent> RidingCamera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ULassoComponent> Lasso;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URiderBalanceComponent> Balance;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USteppeFeedbackComponent> Feedback;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Rider|Animation") FRiderPresentationData PresentationData;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input") TObjectPtr<USteppeInputConfig> InputConfig;
    UPROPERTY(EditDefaultsOnly, Category="Input") float LookSensitivity = 1.f;
    void RefreshInputContext();
    void ResetRidingInput();
    UFUNCTION(BlueprintPure, Category="Riding") FGameplayTag GetRiderStateTag() const;
    UFUNCTION(BlueprintPure, Category="Rider|Presentation") FVector GetLassoHandLocation() const;
    UFUNCTION(BlueprintPure, Category="Rider|Presentation") FVector GetReinHandLocation(bool bLeftHand) const;
    UFUNCTION(BlueprintPure, Category="Rider|Presentation") FVector GetFootLocation(bool bLeftFoot) const;
private:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Sprint(const FInputActionValue& Value);
    void Brake(const FInputActionValue& Value);
    void Interact();
    void ToggleDebug();
    void FocusTarget();
    void BeginLassoAim();
    void EndLassoAim();
    void ThrowLasso();
    void BraceLasso(const FInputActionValue& Value);
    void CaptureHorse();
    void RestartTrial();
    void EnsureInputConfig();
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> PlaceholderRider;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryIdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryWalkAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryRunAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryMountedAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryFallAnimation;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> TemporaryLassoSwingAnimations;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryMountedThrowAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryMountedBraceAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryOnFootThrowAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TemporaryOnFootBraceAnimation;
    FRidingIntent Intent;
};
