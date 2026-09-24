#include "Presentation/RiderAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"

void URiderAnimInstance::ApplyRiderPose(UAnimSequence* Base, UAnimSequence* UpperBody, float PlayRate)
{
    if (!Base) { return; }
    PosePlayRate=PlayRate;
    if (Base!=ActiveBaseSequence)
    {
        if (ActiveBaseMontage) { Montage_Stop(0.f,ActiveBaseMontage); }
        ActiveBaseSequence=Base;
        ActiveBaseMontage=PlaySlotAnimationAsDynamicMontage(Base,TEXT("DefaultSlot"),
            .12f,.14f,PosePlayRate,512);
    }
    else if (ActiveBaseMontage)
    {
        Montage_SetPlayRate(ActiveBaseMontage,PosePlayRate);
        const float ClipLength=Base->GetPlayLength();
        if (ClipLength>KINDA_SMALL_NUMBER && Montage_GetPosition(ActiveBaseMontage)>=ClipLength)
        {
            Montage_SetPosition(ActiveBaseMontage,FMath::Fmod(Montage_GetPosition(ActiveBaseMontage),ClipLength));
        }
    }
    if (UpperBody!=ActiveUpperBodySequence)
    {
        if (ActiveUpperBodyMontage) { Montage_Stop(0.f,ActiveUpperBodyMontage); }
        ActiveUpperBodySequence=UpperBody;
        ActiveUpperBodyMontage=UpperBody?PlaySlotAnimationAsDynamicMontage(
            UpperBody,TEXT("UpperBodySlot"),.12f,.14f,1.f,512):nullptr;
    }
    else if (UpperBody && ActiveUpperBodyMontage && Montage_GetPosition(ActiveUpperBodyMontage)>=UpperBody->GetPlayLength())
    {
        Montage_SetPosition(ActiveUpperBodyMontage,FMath::Fmod(Montage_GetPosition(ActiveUpperBodyMontage),UpperBody->GetPlayLength()));
    }
}
