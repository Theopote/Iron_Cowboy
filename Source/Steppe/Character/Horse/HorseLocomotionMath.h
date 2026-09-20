#pragma once
#include "Character/Horse/HorseLocomotionConfig.h"
namespace SteppeHorseMath
{
    inline float ApproachSpeed(float Current, float Desired, float Acceleration, float Deceleration, float Dt)
    {
        const float Target = FMath::Max(0.f, Desired);
        const float Rate = Target > Current ? Acceleration : Deceleration;
        return FMath::Max(0.f, Current + FMath::Clamp(Target - Current, -FMath::Max(0.f,Rate)*FMath::Max(0.f,Dt), FMath::Max(0.f,Rate)*FMath::Max(0.f,Dt)));
    }
    inline FVector TurnVelocityTowardFacing(FVector CurrentVelocity, FVector Facing, float TargetSpeed, float GripRate, float Dt)
    {
        const FVector Desired=Facing.GetSafeNormal2D();
        if (TargetSpeed<=KINDA_SMALL_NUMBER || Desired.IsNearlyZero()) { return FVector::ZeroVector; }
        const FVector Previous=CurrentVelocity.GetSafeNormal2D();
        if (Previous.IsNearlyZero()) { return Desired*TargetSpeed; }
        const float PreviousYaw=Previous.Rotation().Yaw;
        const float DesiredYaw=Desired.Rotation().Yaw;
        const float Alpha=1.f-FMath::Exp(-FMath::Max(0.f,GripRate)*FMath::Max(0.f,Dt));
        const float NewYaw=PreviousYaw+FMath::FindDeltaAngleDegrees(PreviousYaw,DesiredYaw)*Alpha;
        return FRotator(0.f,NewYaw,0.f).Vector()*TargetSpeed;
    }
    inline EHorseGait SelectGait(float Speed, EHorseGait Previous, const UHorseLocomotionConfig& Config)
    {
        int32 Index = FMath::Clamp(static_cast<int32>(Previous),0,5);
        const float H = FMath::Max(0.f,Config.GaitHysteresis);
        auto Boundary = [&Config](int32 Upper)
        {
            return Upper == 1 ? FMath::Max(0.f, Config.IdleThreshold) :
                .5f*(Config.GetGait(static_cast<EHorseGait>(Upper-1)).TargetSpeed + Config.GetGait(static_cast<EHorseGait>(Upper)).TargetSpeed);
        };
        while (Index < 5 && Speed > Boundary(Index+1) + (Index == 0 ? 0.f : H)) { ++Index; }
        while (Index > 0 && Speed < Boundary(Index) - (Index == 1 ? 0.f : H)) { --Index; }
        return static_cast<EHorseGait>(Index);
    }
}
