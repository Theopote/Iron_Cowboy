#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "SteppeFeedbackComponent.generated.h"

UENUM(BlueprintType)
enum class ESteppeFeedbackEvent : uint8
{
    None, Hoofbeat, LassoSwing, LassoThrow, LassoAttach, LassoRelease, LassoBreak,
    RopeDanger, HorseControlled, HorseCaptured, BalanceWarning, RiderFall, RiderDragged
};

UCLASS(Transient)
class STEPPE_API USteppeProceduralTone : public USoundWaveProcedural
{
    GENERATED_BODY()
public:
    void InitializeTone(float Frequency,float ToneSeconds,float Amplitude,float NoiseMix);
};

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API USteppeFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    USteppeFeedbackComponent();
    virtual void TickComponent(float DeltaSeconds,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;

    float GetHoofbeatInterval(EHorseGait Gait) const;
    float CalculateBreathIntensity(float SpeedNormalized,float StaminaNormalized,EHorseGait Gait) const;
    void EmitEvent(ESteppeFeedbackEvent Event);

    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Feedback") bool bEnablePlaceholderAudio = true;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Feedback",meta=(ClampMin="0",ClampMax="1")) float PlaceholderVolume = .16f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float SpeedNormalized = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float WindIntensity = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float BreathIntensity = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float RopeStress = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float HoofbeatPulse = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float DustPulse = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") ESteppeFeedbackEvent LastEvent = ESteppeFeedbackEvent::None;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 EventCount = 0;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 HoofbeatCount = 0;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 LassoEventCount = 0;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 RiskEventCount = 0;

private:
    float HoofbeatRemaining = 0.f;
    float DangerCueRemaining = 0.f;
    float LastEventAge = 0.f;
    float PreviousSwingPhase = 0.f;
    uint8 PreviousLassoState = 0;
    uint8 PreviousBalanceState = 0;
    void UpdateMovementSignals(float DeltaSeconds);
    void UpdateGameplayEvents(float DeltaSeconds);
    void PlayPlaceholderTone(ESteppeFeedbackEvent Event);
};
