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
    UFUNCTION(BlueprintCallable, Category="Horse|Dynamics") void SetExternalAcceleration(FVector InAcceleration);
    UFUNCTION(BlueprintCallable, Category="Horse|Dynamics") void ClearExternalAcceleration();
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
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Dynamics") float ForwardSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Dynamics") float LateralSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Dynamics") float SlipAngleDegrees = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Dynamics") float EffectiveGripRate = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Dynamics") FVector ExternalAcceleration = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse|Dynamics", meta=(ClampMin="0")) float MaximumExternalAcceleration = 700.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse|Dynamics", meta=(ClampMin="0.1")) float LowSpeedGripRate = 18.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse|Dynamics", meta=(ClampMin="0.1")) float HighSpeedGripRate = 3.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse|Dynamics", meta=(ClampMin="0",ClampMax="2")) float GrassGripMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse|Dynamics", meta=(ClampMin="0",ClampMax="2")) float HardGripMultiplier = 1.05f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") EHorseGait Gait = EHorseGait::Idle;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse") EHorseMovementState HorseState = EHorseMovementState::Grounded;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Horse", meta=(ClampMin="0",ClampMax="2")) float SurfaceMovementMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Riding|Avoidance", meta=(ClampMin="100")) float RiderObstacleProbeDistance = 550.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Riding|Avoidance", meta=(ClampMin="10")) float RiderObstacleProbeRadius = 55.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Riding|Avoidance", meta=(ClampMin="5",ClampMax="80")) float RiderAvoidanceAngle = 32.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Riding|Avoidance", meta=(ClampMin="0",ClampMax="1")) float RiderAvoidanceStrength = .65f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Riding|Avoidance") bool bRiderAvoidingObstacle = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Riding|Avoidance") float RiderAvoidanceTurn = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Riding|Avoidance") float RiderObstacleDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Riding|Avoidance") float RiderAvoidanceSpeedScale = 1.f;
private:
    void UpdateResponse(float DeltaTime, ASteppeHorseCharacter& Horse);
    void ApplyRiderObstacleAvoidance(ASteppeHorseCharacter& Horse);
    bool ProbeRiderPath(ASteppeHorseCharacter& Horse, FVector Direction, float Distance, FHitResult* Hit=nullptr) const;
    float GetSurfaceGripMultiplier() const;
    bool bRiderSource = true;
    bool bExhausted = false;
    float ResponseForward = 0.f;
    float ResponseTurn = 0.f;
    float PreviousAvoidanceTurn = 1.f;
};
