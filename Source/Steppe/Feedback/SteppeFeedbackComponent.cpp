#include "Feedback/SteppeFeedbackComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/RiderBalanceComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Lasso/LassoComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"

void USteppeProceduralTone::InitializeTone(float Frequency,float ToneSeconds,float Amplitude,float NoiseMix)
{
    constexpr int32 ToneSampleRate=22050;
    NumChannels=1;
    SetSampleRate(ToneSampleRate);
    Duration=FMath::Max(.02f,ToneSeconds);
    bLooping=false;
    SoundGroup=SOUNDGROUP_Effects;
    const int32 SampleCount=FMath::Max(1,FMath::RoundToInt(Duration*ToneSampleRate));
    TArray<int16> Samples;
    Samples.SetNumUninitialized(SampleCount);
    uint32 NoiseState=0x51E77E11u;
    for (int32 Index=0; Index<SampleCount; ++Index)
    {
        const float Time=static_cast<float>(Index)/ToneSampleRate;
        const float Envelope=FMath::Square(1.f-static_cast<float>(Index)/SampleCount);
        NoiseState=NoiseState*1664525u+1013904223u;
        const float Noise=(static_cast<float>((NoiseState>>16)&0xffff)/32767.5f)-1.f;
        const float Sine=FMath::Sin(2.f*PI*Frequency*Time);
        const float Mixed=FMath::Lerp(Sine,Noise,FMath::Clamp(NoiseMix,0.f,1.f));
        Samples[Index]=static_cast<int16>(FMath::Clamp(Mixed*Envelope*Amplitude,-1.f,1.f)*32767.f);
    }
    QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()),Samples.Num()*sizeof(int16));
}

USteppeFeedbackComponent::USteppeFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}

float USteppeFeedbackComponent::GetHoofbeatInterval(EHorseGait Gait) const
{
    switch (Gait)
    {
    case EHorseGait::Walk: return .62f;
    case EHorseGait::Trot: return .42f;
    case EHorseGait::Canter: return .31f;
    case EHorseGait::Gallop: return .24f;
    case EHorseGait::Sprint: return .19f;
    default: return 0.f;
    }
}

float USteppeFeedbackComponent::CalculateBreathIntensity(float SpeedValue,float StaminaValue,EHorseGait Gait) const
{
    const float SprintBoost=Gait==EHorseGait::Sprint?.2f:0.f;
    return FMath::Clamp(FMath::Clamp(SpeedValue,0.f,1.f)*.25f+(1.f-FMath::Clamp(StaminaValue,0.f,1.f))*.9f+SprintBoost,0.f,1.f);
}

ESteppeGroundSurface USteppeFeedbackComponent::ResolveGroundSurface(TEnumAsByte<EPhysicalSurface> Surface) const
{
    return Surface==SurfaceType2?ESteppeGroundSurface::Hard:ESteppeGroundSurface::Grass;
}

float USteppeFeedbackComponent::GetSurfaceCadenceScale(ESteppeGroundSurface Surface) const
{
    return Surface==ESteppeGroundSurface::Hard?.92f:1.f;
}

void USteppeFeedbackComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (Dt<=0.f) { return; }
    HoofbeatPulse=FMath::Max(0.f,HoofbeatPulse-Dt*5.f);
    DustPulse=FMath::Max(0.f,DustPulse-Dt*3.f);
    LastEventAge+=Dt;
    DangerCueRemaining=FMath::Max(0.f,DangerCueRemaining-Dt);
    SurfaceProbeRemaining-=Dt;
    BreathCueRemaining-=Dt;
    if (SurfaceProbeRemaining<=0.f) { SurfaceProbeRemaining=.2f; DetectGroundSurface(); }
    UpdateMovementSignals(Dt);
    UpdateGameplayEvents(Dt);
}

void USteppeFeedbackComponent::UpdateMovementSignals(float Dt)
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Horse=Rider && Rider->Riding?Rider->Riding->GetHorse():nullptr;
    auto* Movement=Horse?Cast<UHorseMovementComponent>(Horse->GetCharacterMovement()):nullptr;
    const float TargetSpeed=Movement && Horse->Attributes?FMath::Clamp(Movement->CurrentSpeed/FMath::Max(1.f,Horse->Attributes->MaxSpeed),0.f,1.f):0.f;
    SpeedNormalized=FMath::FInterpTo(SpeedNormalized,TargetSpeed,Dt,4.f);
    WindIntensity=FMath::FInterpTo(WindIntensity,FMath::Square(TargetSpeed),Dt,2.f);
    const float Stamina=Horse && Horse->Attributes?Horse->Attributes->GetStaminaNormalized():1.f;
    const EHorseGait Gait=Movement?Movement->Gait:EHorseGait::Idle;
    BreathIntensity=FMath::FInterpTo(BreathIntensity,CalculateBreathIntensity(TargetSpeed,Stamina,Gait),Dt,3.f);
    const float Interval=GetHoofbeatInterval(Gait)*GetSurfaceCadenceScale(GroundSurface);
    if (Horse && Interval>0.f && Movement->CurrentSpeed>80.f)
    {
        HoofbeatRemaining-=Dt;
        if (HoofbeatRemaining<=0.f)
        {
            HoofbeatRemaining=Interval;
            HoofbeatPulse=1.f;
            DustPulse=GroundSurface==ESteppeGroundSurface::Water?0.f:FMath::Clamp((TargetSpeed-.15f)/.85f,0.f,1.f)*(GroundSurface==ESteppeGroundSurface::Grass?1.f:.3f);
            EmitEvent(ESteppeFeedbackEvent::Hoofbeat);
            SpawnConfiguredDust();
        }
    }
    else { HoofbeatRemaining=0.f; }
    if (BreathIntensity>.32f && BreathCueRemaining<=0.f)
    {
        BreathCueRemaining=FMath::Lerp(2.8f,1.1f,BreathIntensity);
        EmitEvent(ESteppeFeedbackEvent::HorseBreath);
    }
    else if (BreathIntensity<.18f) { BreathCueRemaining=FMath::Min(BreathCueRemaining,.5f); }
}

void USteppeFeedbackComponent::DetectGroundSurface()
{
    const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    const auto* Horse=Rider && Rider->Riding?Rider->Riding->GetHorse():nullptr;
    UWorld* World=GetWorld();
    if (!Horse || !World) { GroundSurface=ESteppeGroundSurface::Grass; return; }
    if (const auto* Movement=Cast<UHorseMovementComponent>(Horse->GetCharacterMovement()); Movement && Movement->SurfaceMovementMultiplier<.99f)
    { GroundSurface=ESteppeGroundSurface::Water; return; }
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeFeedbackSurface),false,Horse);
    Params.bReturnPhysicalMaterial=true;
    Params.AddIgnoredActor(Rider);
    const FVector Start=Horse->GetActorLocation()+FVector(0,0,30.f);
    if (World->LineTraceSingleByChannel(Hit,Start,Start-FVector(0,0,220.f),ECC_Visibility,Params) && Hit.PhysMaterial.IsValid())
    {
        GroundSurface=ResolveGroundSurface(Hit.PhysMaterial->SurfaceType);
    }
    else { GroundSurface=ESteppeGroundSurface::Grass; }
}

void USteppeFeedbackComponent::UpdateGameplayEvents(float Dt)
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    auto* Balance=Rider?Rider->Balance.Get():nullptr;
    if (!Lasso || !Balance) { return; }
    RopeStress=FMath::FInterpTo(RopeStress,FMath::Clamp(Lasso->Tension/1.2f,0.f,1.f),Dt,8.f);
    const uint8 LassoState=static_cast<uint8>(Lasso->State);
    if (LassoState!=PreviousLassoState)
    {
        switch (Lasso->State)
        {
        case ELassoState::Thrown: EmitEvent(ESteppeFeedbackEvent::LassoThrow); break;
        case ELassoState::Attached: EmitEvent(ESteppeFeedbackEvent::LassoAttach); break;
        case ELassoState::Recovering: EmitEvent(Lasso->Feedback.Contains(TEXT("broke"))?ESteppeFeedbackEvent::LassoBreak:ESteppeFeedbackEvent::LassoRelease); break;
        case ELassoState::Subdued: EmitEvent(ESteppeFeedbackEvent::HorseControlled); break;
        case ELassoState::Captured: EmitEvent(ESteppeFeedbackEvent::HorseCaptured); break;
        default: break;
        }
        PreviousLassoState=LassoState;
    }
    if (Lasso->State==ELassoState::Aiming && PreviousSwingPhase<.5f && Lasso->SwingPhase>=.5f) { EmitEvent(ESteppeFeedbackEvent::LassoSwing); }
    PreviousSwingPhase=Lasso->State==ELassoState::Aiming?Lasso->SwingPhase:0.f;
    if (Lasso->Tension>.85f && DangerCueRemaining<=0.f)
    {
        DangerCueRemaining=.45f;
        EmitEvent(ESteppeFeedbackEvent::RopeDanger);
    }
    const uint8 BalanceState=static_cast<uint8>(Balance->State);
    if (BalanceState!=PreviousBalanceState)
    {
        switch (Balance->State)
        {
        case ERiderBalanceState::Warning: EmitEvent(ESteppeFeedbackEvent::BalanceWarning); break;
        case ERiderBalanceState::Falling: EmitEvent(ESteppeFeedbackEvent::RiderFall); break;
        case ERiderBalanceState::Dragged: EmitEvent(ESteppeFeedbackEvent::RiderDragged); break;
        default: break;
        }
        PreviousBalanceState=BalanceState;
    }
}

void USteppeFeedbackComponent::EmitEvent(ESteppeFeedbackEvent Event)
{
    if (Event==ESteppeFeedbackEvent::None) { return; }
    LastEvent=Event;
    LastEventAge=0.f;
    ++EventCount;
    if (Event==ESteppeFeedbackEvent::Hoofbeat) { ++HoofbeatCount; }
    if (Event>=ESteppeFeedbackEvent::LassoSwing && Event<=ESteppeFeedbackEvent::HorseCaptured) { ++LassoEventCount; }
    if (Event==ESteppeFeedbackEvent::RopeDanger || Event>=ESteppeFeedbackEvent::BalanceWarning) { ++RiskEventCount; }
    PlayEventAudio(Event);
}

USoundBase* USteppeFeedbackComponent::ResolveConfiguredSound(ESteppeFeedbackEvent Event) const
{
    switch (Event)
    {
    case ESteppeFeedbackEvent::Hoofbeat: return GroundSurface==ESteppeGroundSurface::Water?Assets.WaterHoof:(GroundSurface==ESteppeGroundSurface::Hard?Assets.HardHoof:Assets.GrassHoof);
    case ESteppeFeedbackEvent::HorseBreath: return Assets.Breath;
    case ESteppeFeedbackEvent::LassoSwing: return Assets.LassoSwing;
    case ESteppeFeedbackEvent::LassoThrow: return Assets.LassoThrow;
    case ESteppeFeedbackEvent::LassoAttach: return Assets.LassoAttach;
    case ESteppeFeedbackEvent::LassoRelease: return Assets.LassoRelease;
    case ESteppeFeedbackEvent::LassoBreak: return Assets.LassoBreak;
    case ESteppeFeedbackEvent::RopeDanger: return Assets.RopeDanger;
    case ESteppeFeedbackEvent::HorseControlled: return Assets.HorseControlled;
    case ESteppeFeedbackEvent::HorseCaptured: return Assets.HorseCaptured;
    case ESteppeFeedbackEvent::BalanceWarning: return Assets.BalanceWarning;
    case ESteppeFeedbackEvent::RiderFall: return Assets.RiderFall;
    case ESteppeFeedbackEvent::RiderDragged: return Assets.RiderDragged;
    default: return nullptr;
    }
}

void USteppeFeedbackComponent::SpawnConfiguredDust()
{
    const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    const auto* Horse=Rider && Rider->Riding?Rider->Riding->GetHorse():nullptr;
    UNiagaraSystem* System=GroundSurface==ESteppeGroundSurface::Water?Assets.WaterHoofSplash.Get():(GroundSurface==ESteppeGroundSurface::Hard?Assets.HardHoofDust.Get():Assets.GrassHoofDust.Get());
    if (!Horse || !System) { return; }
    const FVector Location=Horse->GetActorLocation()-Horse->GetActorForwardVector()*140.f+FVector(0,0,-75.f);
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,System,Location,Horse->GetActorRotation());
}

void USteppeFeedbackComponent::PlayEventAudio(ESteppeFeedbackEvent Event)
{
    UWorld* World=GetWorld();
    if (!World || !World->AllowAudioPlayback()) { return; }
    if (USoundBase* Sound=ResolveConfiguredSound(Event))
    {
        UGameplayStatics::SpawnSound2D(this,Sound,1.f,1.f,0.f,nullptr,false,true);
        return;
    }
    if (!bEnableProceduralFallback) { return; }
    float Frequency=180.f,Seconds=.08f,Noise=.1f,Volume=PlaceholderVolume;
    switch (Event)
    {
    case ESteppeFeedbackEvent::Hoofbeat:
        if (GroundSurface==ESteppeGroundSurface::Water)
        { Frequency=105.f; Seconds=.09f; Noise=.8f; Volume*=.8f; break; }
        Frequency=GroundSurface==ESteppeGroundSurface::Hard?135.f:85.f;
        Seconds=GroundSurface==ESteppeGroundSurface::Hard?.045f:.065f;
        Noise=GroundSurface==ESteppeGroundSurface::Hard?.18f:.5f;
        Volume*=.7f;
        break;
    case ESteppeFeedbackEvent::HorseBreath: Frequency=115.f; Seconds=.2f; Noise=.55f; Volume*=BreathIntensity; break;
    case ESteppeFeedbackEvent::LassoSwing: Frequency=330.f; Seconds=.08f; break;
    case ESteppeFeedbackEvent::LassoThrow: Frequency=470.f; Seconds=.11f; break;
    case ESteppeFeedbackEvent::LassoAttach: Frequency=190.f; Seconds=.14f; Noise=.25f; break;
    case ESteppeFeedbackEvent::LassoBreak: Frequency=75.f; Seconds=.24f; Noise=.75f; break;
    case ESteppeFeedbackEvent::RopeDanger: Frequency=620.f; Seconds=.1f; break;
    case ESteppeFeedbackEvent::BalanceWarning: Frequency=540.f; Seconds=.16f; break;
    case ESteppeFeedbackEvent::RiderFall: case ESteppeFeedbackEvent::RiderDragged: Frequency=65.f; Seconds=.22f; Noise=.65f; break;
    case ESteppeFeedbackEvent::HorseControlled: case ESteppeFeedbackEvent::HorseCaptured: Frequency=260.f; Seconds=.18f; break;
    default: break;
    }
    auto* Tone=NewObject<USteppeProceduralTone>(this);
    Tone->InitializeTone(Frequency,Seconds,.75f,Noise);
    UGameplayStatics::SpawnSound2D(this,Tone,Volume,1.f,0.f,nullptr,false,true);
}
