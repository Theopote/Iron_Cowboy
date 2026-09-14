#include "Character/Rider/RiderBalanceComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

URiderBalanceComponent::URiderBalanceComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}

float URiderBalanceComponent::CalculateLoad(float TensionValue,float LateralValue,float Speed,float TargetStrength,ELassoHitZone Zone) const
{
    const float SpeedFactor=FMath::Clamp((Speed-SafeSpeed)/FMath::Max(1.f,CriticalSpeed-SafeSpeed),0.f,1.f);
    const float ZoneScale=Zone==ELassoHitZone::Head?1.2f:(Zone==ELassoHitZone::Torso?.9f:1.f);
    return FMath::Max(0.f,TensionValue)*FMath::Clamp(LateralValue,0.f,1.f)*SpeedFactor*FMath::Max(.1f,TargetStrength)*ZoneScale;
}

void URiderBalanceComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (Dt<=0.f) { return; }
    if (State==ERiderBalanceState::Dragged) { UpdateDragged(Dt); return; }
    if (State==ERiderBalanceState::Falling) { UpdateFall(Dt); return; }
    if (State==ERiderBalanceState::Recovering)
    {
        StateRemaining-=Dt;
        Balance=FMath::Max(0.f,Balance-RecoveryPerSecond*2.f*Dt);
        if (StateRemaining<=0.f) { State=ERiderBalanceState::Stable; Balance=0.f; Feedback=TEXT("Balance recovered"); }
        return;
    }
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    const bool bOnFoot=Rider && Rider->Riding && !Rider->Riding->IsMounted();
    const bool bHoldingRope=Rider && Rider->Lasso && Rider->Lasso->State==ELassoState::Attached && Rider->Lasso->Target.IsValid();
    if (bOnFoot && bHoldingRope) { UpdateOnFootRope(Dt); return; }
    if (State==ERiderBalanceState::Pulled) { BeginRecovery(TEXT("Rope released - recovering")); return; }
    UpdateMountedBalance(Dt);
}

void URiderBalanceComponent::UpdateMountedBalance(float Dt)
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Horse=Rider && Rider->Riding?Rider->Riding->GetHorse():nullptr;
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    if (!Horse || !Lasso || Lasso->State!=ELassoState::Attached || !Lasso->Target.IsValid())
    {
        CurrentLoad=0.f;
        LateralPull=0.f;
        Balance=FMath::Max(0.f,Balance-RecoveryPerSecond*Dt);
        State=Balance>=WarningThreshold?ERiderBalanceState::Warning:ERiderBalanceState::Stable;
        Feedback=State==ERiderBalanceState::Warning?TEXT("Balance warning - align with the rope"):TEXT("Balance safe");
        return;
    }
    const FVector RopeDirection=(Lasso->Target->GetActorLocation()-Horse->GetActorLocation()).GetSafeNormal2D();
    LateralPull=FMath::Abs(FVector::DotProduct(Horse->GetActorRightVector(),RopeDirection));
    const float Strength=Lasso->Target->Attributes?Lasso->Target->Attributes->Strength:1.f;
    CurrentLoad=FMath::Max(CalculateLoad(Lasso->Tension,LateralPull,Horse->GetVelocity().Size2D(),Strength,Lasso->HitZone),Lasso->ShockLoad*.85f);
    Balance=FMath::Clamp(Balance+(CurrentLoad>=BuildThreshold?CurrentLoad*BuildPerSecond:-RecoveryPerSecond)*Dt,0.f,FallThreshold);
    if (Balance>=FallThreshold-KINDA_SMALL_NUMBER) { TriggerFall(); return; }
    State=Balance>=WarningThreshold?ERiderBalanceState::Warning:ERiderBalanceState::Stable;
    Feedback=State==ERiderBalanceState::Warning?TEXT("BALANCE WARNING - turn toward the rope or slow down"):TEXT("Balance safe");
}

void URiderBalanceComponent::TriggerFall()
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    if (!Rider || !Rider->Riding || !Rider->Riding->IsMounted()) { BeginRecovery(TEXT("Balance recovered")); return; }
    FVector PullDirection=Rider->GetActorRightVector();
    if (Lasso && Lasso->Target.IsValid()) { PullDirection=(Lasso->Target->GetActorLocation()-Rider->GetActorLocation()).GetSafeNormal2D(); }
    if (!Rider->Riding->ForceDismount(PullDirection*FallHorizontalSpeed+FVector(0,0,FallUpSpeed)))
    {
        BeginRecovery(TEXT("Fall avoided"));
        return;
    }
    State=ERiderBalanceState::Falling;
    StateRemaining=FallSeconds;
    Feedback=TEXT("FALL - release LMB to drop the rope");
    const bool bCanDrag=Lasso && Lasso->State==ELassoState::Attached && Lasso->Target.IsValid()
        && FVector::Dist2D(Rider->GetActorLocation(),Lasso->Target->GetActorLocation())<=MaximumDraggedDistance;
    if (bCanDrag)
    {
        State=ERiderBalanceState::Dragged;
        DraggedRemaining=MaximumDraggedSeconds;
        Feedback=TEXT("DRAGGED - keep Space held to control, or LMB to release");
    }
    else if (Lasso) { Lasso->Release(); }
}

void URiderBalanceComponent::UpdateFall(float Dt)
{
    StateRemaining-=Dt;
    if (StateRemaining<=0.f) { BeginRecovery(TEXT("Recovering from fall")); }
}

void URiderBalanceComponent::UpdateDragged(float Dt)
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    if (!Rider || !Lasso || Lasso->State!=ELassoState::Attached || !Lasso->Target.IsValid())
    {
        BeginRecovery(TEXT("Rope released - recovering"));
        return;
    }
    DraggedRemaining=FMath::Max(0.f,DraggedRemaining-Dt);
    const FVector PullDirection=(Lasso->Target->GetActorLocation()-Rider->GetActorLocation()).GetSafeNormal2D();
    auto* Movement=Rider->GetCharacterMovement();
    Movement->Velocity=PullDirection*DragSpeed+FVector(0,0,Movement->Velocity.Z);
    Feedback=TEXT("DRAGGED - keep Space held to control, or LMB to release");
    if (DraggedRemaining<=0.f)
    {
        Movement->SetMovementMode(MOVE_Walking);
        State=ERiderBalanceState::Pulled;
        Balance=FMath::Min(Balance,WarningThreshold);
        Feedback=TEXT("BACK ON YOUR FEET - run with the horse and hold Space");
    }
}

void URiderBalanceComponent::UpdateOnFootRope(float Dt)
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    if (!Rider || !Lasso || Lasso->State!=ELassoState::Attached || !Lasso->Target.IsValid())
    {
        BeginRecovery(TEXT("Rope released - recovering"));
        return;
    }
    const FVector PullDirection=(Lasso->Target->GetActorLocation()-Rider->GetActorLocation()).GetSafeNormal2D();
    const float Pull=FMath::Clamp((Lasso->Tension-OnFootPullThreshold)/FMath::Max(.01f,1.5f-OnFootPullThreshold),0.f,1.f);
    auto* Movement=Rider->GetCharacterMovement();
    FVector Horizontal=FVector::VectorPlaneProject(Movement->Velocity,FVector::UpVector);
    Horizontal+=PullDirection*OnFootPullAcceleration*Pull*Dt;
    Horizontal=Horizontal.GetClampedToMaxSize(MaximumOnFootPullSpeed*FMath::Lerp(.45f,1.f,Pull));
    Movement->Velocity=Horizontal+FVector(0,0,Movement->Velocity.Z);
    State=ERiderBalanceState::Pulled;
    CurrentLoad=Pull;
    LateralPull=0.f;
    Balance=FMath::Max(0.f,Balance-RecoveryPerSecond*Dt);
    Feedback=Pull>.5f?TEXT("PULLED ON FOOT - run with the horse and hold Space"):TEXT("ON-FOOT ROPE CONTROL - hold Space and keep tension green");
}

void URiderBalanceComponent::BeginRecovery(const TCHAR* Message)
{
    State=ERiderBalanceState::Recovering;
    StateRemaining=PostFallRecoverySeconds;
    DraggedRemaining=0.f;
    CurrentLoad=0.f;
    LateralPull=0.f;
    Feedback=Message;
}
