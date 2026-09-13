#include "Capture/HorseTrustComponent.h"
#include "AI/HorseBrainComponent.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"

UHorseTrustComponent::UHorseTrustComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
    PrimaryComponentTick.TickInterval=.1f;
}

void UHorseTrustComponent::BeginSecured(ASteppeRiderCharacter* Rider)
{
    Interactor=Rider;
    State=EPostCaptureState::Secured;
    DistanceToRider=Rider?FVector::Dist2D(GetOwner()->GetActorLocation(),Rider->GetActorLocation()):0.f;
    RiderApproachSpeed=0.f;
    Pressure=0.f;
    CalmProgress=0.f;
    Trust=0.f;
    bFirstContact=false;
    RejectionRemaining=0.f;
    Feedback=TEXT("Slow down and dismount");
}

void UHorseTrustComponent::AdvanceApproach(float Dt, float Distance, float ApproachSpeed, bool bRiderMounted)
{
    if (State==EPostCaptureState::Inactive || State==EPostCaptureState::FirstContact || Dt<=0.f) { return; }
    DistanceToRider=FMath::Max(0.f,Distance);
    RiderApproachSpeed=ApproachSpeed;
    if (bRiderMounted)
    {
        State=EPostCaptureState::Secured;
        CalmProgress=0.f;
        Pressure=FMath::Max(Pressure-Dt*.25f,0.f);
        Feedback=TEXT("Slow down and dismount");
        return;
    }
    RejectionRemaining=FMath::Max(0.f,RejectionRemaining-Dt);
    if (DistanceToRider<=AwarenessRadius && RiderApproachSpeed>=RushApproachSpeed)
    {
        State=EPostCaptureState::Rejected;
        Pressure=1.f;
        CalmProgress=0.f;
        RejectionRemaining=FMath::Max(RejectionRemaining,RejectionCooldown);
        Feedback=TEXT("Too fast - stop and give the horse space");
        if (auto* Horse=Cast<ASteppeWildHorseCharacter>(GetOwner()))
        {
            const FVector Away=(Horse->GetActorLocation()-(Interactor.IsValid()?Interactor->GetActorLocation():Horse->GetActorLocation()-Horse->GetActorForwardVector())).GetSafeNormal2D();
            Horse->Brain->RequestCapturedRetreat(Away.IsNearlyZero()?Horse->GetActorForwardVector():Away,RetreatSpeed,RejectionCooldown);
        }
        return;
    }
    if (RejectionRemaining>0.f)
    {
        State=EPostCaptureState::Rejected;
        Pressure=FMath::Max(0.f,Pressure-Dt/RejectionCooldown);
        Feedback=TEXT("Give the horse space");
        return;
    }
    Pressure=FMath::Max(0.f,Pressure-Dt*.5f);
    State=EPostCaptureState::CalmApproach;
    if (DistanceToRider>AwarenessRadius)
    {
        CalmProgress=FMath::Max(0.f,CalmProgress-Dt/FMath::Max(.1f,CalmHoldSeconds));
        Feedback=TEXT("Approach slowly");
        return;
    }
    if (DistanceToRider<=ContactDistance && RiderApproachSpeed<=SafeApproachSpeed)
    {
        CalmProgress=FMath::Clamp(CalmProgress+Dt/FMath::Max(.1f,CalmHoldSeconds),0.f,1.f);
        if (CalmProgress>=1.f)
        {
            State=EPostCaptureState::ReadyForContact;
            Feedback=TEXT("Press E for first contact");
        }
        else { Feedback=TEXT("Hold still and let the horse settle"); }
    }
    else
    {
        CalmProgress=FMath::Max(0.f,CalmProgress-Dt*.5f/FMath::Max(.1f,CalmHoldSeconds));
        Feedback=RiderApproachSpeed>SafeApproachSpeed?TEXT("Approach more slowly"):TEXT("Move a little closer");
    }
}

bool UHorseTrustComponent::TryFirstContact(ASteppeRiderCharacter* Rider)
{
    if (!Rider || Rider!=Interactor.Get() || Rider->Riding->IsMounted() || State!=EPostCaptureState::ReadyForContact
        || FVector::Dist2D(GetOwner()->GetActorLocation(),Rider->GetActorLocation())>ContactDistance)
    {
        Feedback=TEXT("Let the horse settle before contact");
        return false;
    }
    State=EPostCaptureState::FirstContact;
    bFirstContact=true;
    Trust=FMath::Clamp(FirstContactTrust,0.f,100.f);
    CalmProgress=1.f;
    Pressure=0.f;
    Feedback=TEXT("FIRST CONTACT | the horse accepts you");
    return true;
}

void UHorseTrustComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (State==EPostCaptureState::Inactive || State==EPostCaptureState::FirstContact) { return; }
    auto* Rider=Interactor.Get();
    if (!Rider) { Feedback=TEXT("Rider unavailable"); return; }
    const FVector ToHorse=(GetOwner()->GetActorLocation()-Rider->GetActorLocation()).GetSafeNormal2D();
    const AActor* MotionSource=Rider->GetAttachParentActor()?Rider->GetAttachParentActor():Rider;
    const float ApproachSpeed=FVector::DotProduct(MotionSource->GetVelocity(),ToHorse);
    AdvanceApproach(Dt,FVector::Dist2D(GetOwner()->GetActorLocation(),Rider->GetActorLocation()),ApproachSpeed,Rider->Riding->IsMounted());
}
