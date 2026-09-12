#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WildHorseConfig.generated.h"

UCLASS(BlueprintType)
class STEPPE_API UWildHorseConfig : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="0.02")) float DecisionInterval = .1f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="1")) float NoticeDistance = 3500.f;
    // Existing assets retain this distance as the slow-approach yielding boundary.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="1", DisplayName="Yield Distance")) float FlightDistance = 900.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="1")) float PanicDistance = 350.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="0")) float ApproachDeadZone = 20.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="State", meta=(ClampMin="0")) float PressureReleaseSeconds = 2.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="State", meta=(ClampMin="1")) float YieldSpeed = 220.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0")) float CohesionWeight = .3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0")) float AlignmentWeight = .25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0")) float SeparationWeight = 2.4f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0")) float FlightAlignmentWeight = .45f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0")) float IndividualSteeringDegrees = 18.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Herd", meta=(ClampMin="0")) float ReactionTimeVariation = .22f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="1")) float DynamicAvoidanceDistance = 550.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="0")) float DynamicAvoidanceWeight = 1.8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="0.1")) float BlockedTurnSeconds = 1.1f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="1")) float FastApproachDistance = 2500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="1")) float FastClosingSpeed = 700.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sensing", meta=(ClampMin="0")) float ThreatMemorySeconds = 3.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Awareness", meta=(ClampMin="0.01")) float AwarenessRiseRate = .35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Awareness", meta=(ClampMin="0.01")) float AwarenessDecayRate = .18f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Awareness", meta=(ClampMin="0",ClampMax="1")) float AlertThreshold = .25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Awareness", meta=(ClampMin="0",ClampMax="1")) float FlightThreshold = .75f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Awareness", meta=(ClampMin="0",ClampMax="1")) float CalmThreshold = .1f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="State", meta=(ClampMin="0")) float MinimumAlertSeconds = .8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="State", meta=(ClampMin="0")) float MinimumFlightSeconds = 3.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="State", meta=(ClampMin="0")) float RecoverySeconds = 4.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roaming", meta=(ClampMin="0")) float RoamRadius = 1500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roaming", meta=(ClampMin="1")) float ArrivalRadius = 160.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roaming", meta=(ClampMin="0")) float PauseSeconds = 2.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roaming", meta=(ClampMin="1")) float RoamGoalTimeout = 30.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roaming", meta=(ClampMin="1")) float RoamSpeed = 180.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="1")) float FlightSpeed = 1200.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="1")) float EscapeLookAhead = 2000.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="1")) float FullTurnAngle = 45.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="1")) float ProbeDistance = 450.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="0.1")) float ProbeSeconds = 1.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="1")) float ProbeRadius = 60.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="1")) float GroundProbeDepth = 500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="10")) float GroundSampleSpacing = 100.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="0")) float MaximumGroundDrop = 60.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Steering", meta=(ClampMin="0")) float BrakeSafetyDistance = 150.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roaming") int32 RandomSeed = 1729;
};
