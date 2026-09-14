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
struct FInputActionValue;
UCLASS()
class STEPPE_API ASteppeRiderCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ASteppeRiderCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void PawnClientRestart() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URidingComponent> Riding;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URidingCameraComponent> RidingCamera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ULassoComponent> Lasso;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URiderBalanceComponent> Balance;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USteppeFeedbackComponent> Feedback;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input") TObjectPtr<USteppeInputConfig> InputConfig;
    UPROPERTY(EditDefaultsOnly, Category="Input") float LookSensitivity = 1.f;
    void RefreshInputContext();
    void ResetRidingInput();
    UFUNCTION(BlueprintPure, Category="Riding") FGameplayTag GetRiderStateTag() const;
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
    FRidingIntent Intent;
};
