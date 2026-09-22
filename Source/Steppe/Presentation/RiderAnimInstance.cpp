#include "Presentation/RiderAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"

void URiderAnimInstance::ApplyRiderPose(UAnimSequence* Base, UAnimSequence* UpperBody, float PlayRate)
{
    if (!Base) { return; }
    PosePlayRate=PlayRate;
    if (Base!=ActiveBaseSequence)
    {
        StopSlotAnimation(.12f,TEXT("DefaultSlot"));
        ActiveBaseSequence=Base;
        ActiveBaseMontage=PlaySlotAnimationAsDynamicMontage(Base,TEXT("DefaultSlot"),
            .12f,.14f,PosePlayRate,MAX_int32);
    }
    else if (ActiveBaseMontage)
    {
        Montage_SetPlayRate(ActiveBaseMontage,PosePlayRate);
    }
    if (UpperBody!=ActiveUpperBodySequence)
    {
        StopSlotAnimation(.12f,TEXT("UpperBodySlot"));
        ActiveUpperBodySequence=UpperBody;
        ActiveUpperBodyMontage=UpperBody?PlaySlotAnimationAsDynamicMontage(
            UpperBody,TEXT("UpperBodySlot"),.12f,.14f,1.f,MAX_int32):nullptr;
    }
}
