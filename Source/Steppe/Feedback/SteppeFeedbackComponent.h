#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "Engine/EngineTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "SteppeFeedbackComponent.generated.h"

class USoundBase;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class ESteppeGroundSurface : uint8
{
    Grass, Hard, Water
};

UENUM(BlueprintType)
enum class ESteppeFeedbackEvent : uint8
{
    None, Hoofbeat, HorseBreath, LassoSwing, LassoThrow, LassoAttach, LassoRelease, LassoBreak,
    RopeDanger, HorseControlled, HorseCaptured, BalanceWarning, RiderFall, RiderDragged
};

USTRUCT(BlueprintType)
struct STEPPE_API FSteppeFeedbackAssets
{
    GENERATED_BODY()
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> GrassHoof;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> HardHoof;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> WaterHoof;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> Breath;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> LassoSwing;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> LassoThrow;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> LassoAttach;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> LassoRelease;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> LassoBreak;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> RopeDanger;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> HorseControlled;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> HorseCaptured;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> BalanceWarning;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> RiderFall;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<USoundBase> RiderDragged;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<UNiagaraSystem> GrassHoofDust;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<UNiagaraSystem> HardHoofDust;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TObjectPtr<UNiagaraSystem> WaterHoofSplash;
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
    ESteppeGroundSurface ResolveGroundSurface(TEnumAsByte<EPhysicalSurface> Surface) const;
    float GetSurfaceCadenceScale(ESteppeGroundSurface Surface) const;
    void EmitEvent(ESteppeFeedbackEvent Event);

    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Feedback") bool bEnableProceduralFallback = true;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Feedback",meta=(ClampMin="0",ClampMax="1")) float PlaceholderVolume = .16f;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Feedback") FSteppeFeedbackAssets Assets;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float SpeedNormalized = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float WindIntensity = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float BreathIntensity = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float RopeStress = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float HoofbeatPulse = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") float DustPulse = 0.f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") ESteppeGroundSurface GroundSurface = ESteppeGroundSurface::Grass;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") ESteppeFeedbackEvent LastEvent = ESteppeFeedbackEvent::None;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 EventCount = 0;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 HoofbeatCount = 0;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 LassoEventCount = 0;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Feedback") int32 RiskEventCount = 0;

private:
    float HoofbeatRemaining = 0.f;
    float DangerCueRemaining = 0.f;
    float SurfaceProbeRemaining = 0.f;
    float BreathCueRemaining = 0.f;
    float LastEventAge = 0.f;
    float PreviousSwingPhase = 0.f;
    uint8 PreviousLassoState = 0;
    uint8 PreviousBalanceState = 0;
    void UpdateMovementSignals(float DeltaSeconds);
    void UpdateGameplayEvents(float DeltaSeconds);
    void DetectGroundSurface();
    USoundBase* ResolveConfiguredSound(ESteppeFeedbackEvent Event) const;
    void SpawnConfiguredDust();
    void PlayEventAudio(ESteppeFeedbackEvent Event);
};
