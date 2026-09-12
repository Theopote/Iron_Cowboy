#include "Lasso/LassoComponent.h"
#include "AI/HorseBrainComponent.h"
#include "AI/SteppeHerdManager.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Game/SteppeGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Core/SteppeGameplayTags.h"

ULassoComponent::ULassoComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
}

bool ULassoComponent::BeginAim()
{
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr;
    const auto* Herd=Mode?Mode->HerdManager.Get():nullptr;
    return BeginAimForTarget(Herd?Herd->FocusedHorse.Get():nullptr,Herd && Herd->bTargetIsolated);
}

bool ULassoComponent::BeginAimForTarget(ASteppeWildHorseCharacter* NewTarget, bool bIsolated)
{
    if (State==ELassoState::Attached) { Feedback=TEXT("Release the attached lasso first"); return false; }
    if (State==ELassoState::Thrown || State==ELassoState::Recovering) { return false; }
    if (!IsValid(NewTarget)) { Feedback=TEXT("Select a target with Q"); return false; }
    if (!bIsolated) { Feedback=TEXT("Isolate the target before throwing"); return false; }
    Target=NewTarget;
    State=ELassoState::Aiming;
    Feedback=TEXT("Aim and press Left Mouse to throw");
    return true;
}

void ULassoComponent::CancelAim()
{
    if (State==ELassoState::Aiming)
    {
        State=ELassoState::Stored;
        Target.Reset();
        Feedback=TEXT("Lasso stored");
    }
}

bool ULassoComponent::Throw()
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    auto* Controller=Rider?Cast<APlayerController>(Rider->GetController()):nullptr;
    if (!Controller) { return false; }
    FVector Origin; FRotator Rotation;
    Controller->GetPlayerViewPoint(Origin,Rotation);
    return ThrowFrom(Origin,Rotation.Vector());
}

bool ULassoComponent::ThrowFrom(FVector Origin, FVector Direction)
{
    if (State!=ELassoState::Aiming || !Target.IsValid()) { Feedback=TEXT("Hold Right Mouse to aim first"); return false; }
    RopeStart=Origin;
    LoopLocation=Origin;
    ThrowDirection=Direction.GetSafeNormal();
    TravelDistance=0.f;
    State=ELassoState::Thrown;
    Feedback=TEXT("Lasso in flight");
    return true;
}

void ULassoComponent::StartRecovery(const TCHAR* Message)
{
    if (auto* Horse=Target.Get()) { Horse->Brain->SetLassoed(false); }
    Target.Reset();
    State=ELassoState::Recovering;
    RecoveryRemaining=FMath::Max(.1f,RecoverySeconds);
    Feedback=Message;
}

void ULassoComponent::Release()
{
    if (State==ELassoState::Attached) { StartRecovery(TEXT("Lasso released")); }
}

FGameplayTag ULassoComponent::GetStateTag() const
{
    switch (State)
    {
    case ELassoState::Aiming: return SteppeTags::Lasso_State_Aiming;
    case ELassoState::Thrown: return SteppeTags::Lasso_State_Thrown;
    case ELassoState::Attached: return SteppeTags::Lasso_State_Attached;
    case ELassoState::Recovering: return SteppeTags::Lasso_State_Recovering;
    default: return SteppeTags::Lasso_State_Stored;
    }
}

void ULassoComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (State==ELassoState::Thrown || State==ELassoState::Attached)
    {
        RopeStart=GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*55.f+GetOwner()->GetActorRightVector()*40.f+FVector(0,0,115);
    }
    if (State==ELassoState::Recovering)
    {
        RecoveryRemaining-=Dt;
        if (RecoveryRemaining<=0.f) { State=ELassoState::Stored; Feedback=TEXT("Lasso ready"); }
        return;
    }
    if (State==ELassoState::Attached)
    {
        if (auto* Horse=Target.Get())
        {
            LoopLocation=Horse->GetActorLocation()+FVector(0,0,90);
            if (FVector::Dist(RopeStart,LoopLocation)>MaximumRange*1.1f) { StartRecovery(TEXT("Rope broke - recovering")); }
        }
        else { StartRecovery(TEXT("Target lost")); }
        return;
    }
    if (State!=ELassoState::Thrown) { return; }
    if (!Target.IsValid()) { StartRecovery(TEXT("Target lost")); return; }

    const float Step=FMath::Min(ThrowSpeed*Dt,MaximumRange-TravelDistance);
    const FVector Previous=LoopLocation;
    const FVector Next=Previous+ThrowDirection*FMath::Max(0.f,Step);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeLasso),false,GetOwner());
    if (const auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
    {
        if (Rider->Riding && Rider->Riding->GetHorse()) { Params.AddIgnoredActor(Rider->Riding->GetHorse()); }
    }
    FCollisionObjectQueryParams Pawns;
    Pawns.AddObjectTypesToQuery(ECC_Pawn);
    Pawns.AddObjectTypesToQuery(ECC_WorldStatic);
    Pawns.AddObjectTypesToQuery(ECC_WorldDynamic);
    FHitResult Hit;
    if (GetWorld()->SweepSingleByObjectType(Hit,Previous,Next,FQuat::Identity,Pawns,FCollisionShape::MakeSphere(CaptureRadius),Params))
    {
        LoopLocation=Hit.ImpactPoint;
        if (Hit.GetActor()==Target.Get())
        {
            State=ELassoState::Attached;
            Target->Brain->SetLassoed(true);
            Feedback=TEXT("LASSO ATTACHED - Left Mouse to release");
        }
        else { StartRecovery(TEXT("Lasso blocked - recovering")); }
        return;
    }
    LoopLocation=Next;
    TravelDistance+=Step;
    if (TravelDistance>=MaximumRange-KINDA_SMALL_NUMBER) { StartRecovery(TEXT("Missed - recovering")); }
}

void ULassoComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Horse=Target.Get()) { Horse->Brain->SetLassoed(false); }
    Target.Reset();
    Super::EndPlay(Reason);
}
