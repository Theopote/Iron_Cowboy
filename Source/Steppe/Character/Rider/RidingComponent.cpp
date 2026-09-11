#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Steppe.h"
URidingComponent::URidingComponent() { PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickGroup=TG_PrePhysics; }
void URidingComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (auto* Horse=MountedHorse.Get()) { CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->SetRiderIntent(Intent); }
}
bool URidingComponent::TryMount(ASteppeHorseCharacter* Horse)
{
    auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    if (!Rider || !IsValid(Horse) || !Horse->bCanBeMounted || IsMounted() || Horse->MountedRider.IsValid() || FVector::Dist(Rider->GetActorLocation(),Horse->GetActorLocation())>MountDistance || Horse->GetVelocity().Size2D()>DismountMaxSpeed)
    {
        UE_LOG(LogSteppeRiding,Display,TEXT("Mount refused: invalid/occupied target, distance or speed limit.")); return false;
    }
    MountedHorse=Horse; Horse->MountedRider=Rider;
    Rider->ResetRidingInput();
    Rider->GetCharacterMovement()->StopMovementImmediately();
    Rider->GetCharacterMovement()->DisableMovement();
    Rider->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Horse->GetMesh()->DoesSocketExist(Horse->RiderSocket))
    {
        Rider->AttachToComponent(Horse->GetMesh(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,Horse->RiderSocket);
    }
    else
    {
        Rider->AttachToComponent(Horse->GetRootComponent(),FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Rider->SetActorRelativeTransform(Horse->FallbackSeat);
        UE_LOG(LogSteppeRiding,Warning,TEXT("RiderSeat socket missing on %s; using editable fallback seat."),*Horse->GetName());
    }
    Horse->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
    Horse->OnDestroyed.AddDynamic(this,&URidingComponent::OnHorseDestroyed);
    Rider->RefreshInputContext();
    return true;
}
void URidingComponent::Dismount()
{
    auto* Horse=MountedHorse.Get(); auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner());
    if (!Horse || !Rider) { return; }
    if (Horse->GetVelocity().Size2D()>DismountMaxSpeed) { UE_LOG(LogSteppeRiding,Display,TEXT("Dismount refused: slow below %.0f cm/s."),DismountMaxSpeed); return; }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeDismount),false,Rider); Params.AddIgnoredActor(Horse);
    const float Radius=Rider->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Half=Rider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector Exit=FVector::ZeroVector; bool Found=false;
    for (float Side : {1.f,-1.f})
    {
        const FVector Candidate=Horse->GetActorLocation()+Horse->GetActorRightVector()*DismountOffset*Side;
        FHitResult Floor;
        if (GetWorld()->LineTraceSingleByChannel(Floor,Candidate+FVector(0,0,150),Candidate-FVector(0,0,400),ECC_Visibility,Params) && Floor.ImpactNormal.Z>=Rider->GetCharacterMovement()->GetWalkableFloorZ())
        {
            Exit=Floor.ImpactPoint+FVector(0,0,Half+3);
            if (!GetWorld()->OverlapBlockingTestByChannel(Exit,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,Half),Params)) { Found=true; break; }
        }
    }
    if (!Found) { UE_LOG(LogSteppeRiding,Display,TEXT("Dismount refused: no clear, walkable landing space.")); return; }
    Horse->OnDestroyed.RemoveDynamic(this,&URidingComponent::OnHorseDestroyed);
    Horse->GetCharacterMovement()->RemoveTickPrerequisiteComponent(this);
    CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearIntent();
    Horse->MountedRider.Reset(); MountedHorse.Reset();
    Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Rider->SetActorLocationAndRotation(Exit,FRotator(0,Horse->GetActorRotation().Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
    Rider->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Rider->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Rider->ResetRidingInput(); Rider->RefreshInputContext();
}
void URidingComponent::OnHorseDestroyed(AActor* Actor)
{
    MountedHorse.Reset(); Intent.Reset();
    if (auto* Rider=Cast<ASteppeRiderCharacter>(GetOwner()))
    {
        Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Rider->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Rider->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        Rider->ResetRidingInput(); Rider->RefreshInputContext();
    }
}
void URidingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Horse=MountedHorse.Get())
    {
        Horse->OnDestroyed.RemoveDynamic(this,&URidingComponent::OnHorseDestroyed);
        Horse->GetCharacterMovement()->RemoveTickPrerequisiteComponent(this);
        CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->ClearIntent();
        Horse->MountedRider.Reset();
    }
    Super::EndPlay(Reason);
}
