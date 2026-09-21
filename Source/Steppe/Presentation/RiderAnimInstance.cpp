#include "Presentation/RiderAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"

void URiderAnimInstance::ApplyRiderPose(UAnimSequence* Desired, float PlayRate)
{
    if (!Desired) { return; }
    PosePlayRate=PlayRate;
    if (Desired!=ActiveSequence)
    {
        StopSlotAnimation(.12f,TEXT("DefaultSlot"));
        ActiveSequence=Desired;
        ActiveMontage=PlaySlotAnimationAsDynamicMontage(Desired,TEXT("DefaultSlot"),
            .12f,.14f,PosePlayRate,MAX_int32);
    }
    else if (ActiveMontage)
    {
        Montage_SetPlayRate(ActiveMontage,PosePlayRate);
    }
}
