#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "Character/Rider/RidingIntent.h"
#include "HorseMovementComponent.generated.h"
class ASteppeHorseCharacter;
UCLASS()
class STEPPE_API UHorseMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    UHorseMovementComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
    virtual void PhysicsRotation(float DeltaTime) override;
    UFUNCTION(BlueprintCallable) void SetRiderIntent(const FRidingIntent& Intent);
    UFUNCTION(BlueprintCallable) void SetHorseIntent(const FHorseMovementIntent& Intent);
    UFUNCTION(BlueprintCallable) void ClearIntent();
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") FHorseMovementIntent HorseIntent;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") FRidingIntent RiderIntent;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float DesiredSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float CurrentSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float DesiredHeading = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float CurrentHeading = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float EffectiveTurnRate = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float TurnStress = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float ActualAcceleration = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") float GroundSlope = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") EHorseGait Gait = EHorseGait::Idle;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") EHorseMovementState HorseState = EHorseMovementState::Grounded;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse", meta=(ClampMin="0",ClampMax="2")) float SurfaceMovementMultiplier = 1.f;
private:
    void UpdateResponse(float DeltaTime, ASteppeHorseCharacter& Horse);
    bool bRiderSource = true;
    bool bExhausted = false;
    float ResponseForward = 0.f;
    float ResponseTurn = 0.f;
};
