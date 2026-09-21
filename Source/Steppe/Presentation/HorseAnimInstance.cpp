#include "Presentation/HorseAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"

void UHorseAnimInstance::ApplyHorseData(const FHorseAnimationData& Data,
    UAnimSequence* Idle, UAnimSequence* Walk, UAnimSequence* Gallop, UAnimSequence* Stop, UAnimSequence* Struggle)
{
    HorseData=Data;
    ActiveGait=Data.Gait;
    UAnimSequence* Desired=nullptr;
    if (Data.bStruggling && Struggle)
    {
        Desired=Struggle;
        LocomotionPlayRate=FMath::Lerp(.72f,1.05f,Data.ExternalForceAmount);
    }
    else if (Data.bStopping && Stop)
    {
        Desired=Stop;
        LocomotionPlayRate=1.f;
    }
    else if (Data.Gait==EHorseGait::Walk || Data.Gait==EHorseGait::Trot || Data.bStarting)
    {
        Desired=Walk;
        LocomotionPlayRate=FMath::Clamp(Data.Speed/260.f,.65f,1.6f);
    }
    else if (Data.Gait!=EHorseGait::Idle)
    {
        Desired=Gallop;
        LocomotionPlayRate=FMath::Clamp(Data.Speed/1100.f,.7f,1.5f);
    }
    else
    {
        LocomotionPlayRate=1.f;
    }

    if (Desired!=ActiveSequence)
    {
        // Idle is the AnimGraph base pose. Locomotion montages blend through its slot.
        StopSlotAnimation(.16f,TEXT("DefaultSlot"));
        ActiveSequence=Desired;
        ActiveMontage=Desired?PlaySlotAnimationAsDynamicMontage(Desired,TEXT("DefaultSlot"),
            .16f,.16f,LocomotionPlayRate,MAX_int32):nullptr;
    }
    else if (ActiveMontage)
    {
        Montage_SetPlayRate(ActiveMontage,LocomotionPlayRate);
    }
}
