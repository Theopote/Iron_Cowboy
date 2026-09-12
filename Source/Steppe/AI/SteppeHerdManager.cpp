#include "AI/SteppeHerdManager.h"
#include "AI/HorseBrainComponent.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASteppeHerdManager::ASteppeHerdManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .2f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("HerdRoot")));
    HorseClass = ASteppeWildHorseCharacter::StaticClass();
}

void ASteppeHerdManager::BeginPlay()
{
    Super::BeginPlay();
    EnsureMembersSpawned();
}

void ASteppeHerdManager::EnsureMembersSpawned()
{
    if (!Members.IsEmpty()) { return; }
    static const FVector2D Pattern[] = {
        {0,0}, {-.75f,-1}, {-.75f,1}, {.75f,-.65f}, {.75f,.65f},
        {-1.5f,-.35f}, {-1.5f,.35f}, {1.5f,-1.25f}, {1.5f,1.25f},
        {0,-1.75f}, {0,1.75f}, {1.75f,0}
    };
    const int32 Count = FMath::Clamp(HerdSize, 1, UE_ARRAY_COUNT(Pattern));
    for (int32 Index=0; Index<Count; ++Index)
    {
        const FVector LocalOffset(Pattern[Index].X*FormationSpacing,Pattern[Index].Y*FormationSpacing,0.f);
        const FTransform SpawnTransform(GetActorRotation(),GetActorLocation()+GetActorRotation().RotateVector(LocalOffset));
        auto* Horse=GetWorld()->SpawnActorDeferred<ASteppeWildHorseCharacter>(HorseClass,SpawnTransform,this,nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (Horse)
        {
            Horse->Brain->SetHerdIdentity(Index,HerdSeed);
            Horse->FinishSpawning(SpawnTransform);
            Members.Add(Horse);
            Horse->Brain->SetThreatTarget(ThreatTarget.Get());
        }
    }
}

void ASteppeHerdManager::SetThreatTarget(AActor* Target)
{
    ThreatTarget = Target;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& MemberPtr : Members)
    {
        if (auto* Member=MemberPtr.Get()) { Member->Brain->SetThreatTarget(Target); }
    }
}

void ASteppeHerdManager::Tick(float Dt)
{
    Super::Tick(Dt);
    Members.RemoveAll([](const TObjectPtr<ASteppeWildHorseCharacter>& Horse) { return !IsValid(Horse); });
    if (Members.IsEmpty()) { HerdCenter=FVector::ZeroVector; AverageVelocity=FVector::ZeroVector; return; }

    HerdCenter=FVector::ZeroVector;
    AverageVelocity=FVector::ZeroVector;
    MinimumMemberSpacing=Members.Num()>1?BIG_NUMBER:0.f;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : Members)
    {
        const auto* Horse=HorsePtr.Get(); HerdCenter+=Horse->GetActorLocation(); AverageVelocity+=Horse->GetVelocity();
    }
    HerdCenter/=Members.Num();
    AverageVelocity/=Members.Num();

    TArray<ASteppeWildHorseCharacter*> Sources;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : Members)
    {
        auto* Horse=HorsePtr.Get();
        if (Horse->Brain->State==EWildHorseState::Fleeing && Horse->Brain->bThreatVisible) { Sources.Add(Horse); }
        FVector Separation=FVector::ZeroVector;
        int32 Neighbors=0;
        float StrongestCrowding=0.f;
        for (const TObjectPtr<ASteppeWildHorseCharacter>& OtherPtr : Members)
        {
            const auto* Other=OtherPtr.Get();
            if (Other==Horse) { continue; }
            const FVector Away=Horse->GetActorLocation()-Other->GetActorLocation();
            const float Distance=Away.Size2D();
            MinimumMemberSpacing=FMath::Min(MinimumMemberSpacing,Distance);
            if (Distance<=NeighborRadius) { ++Neighbors; }
            if (Distance>1.f && Distance<SeparationDistance)
            {
                const float Crowding=1.f-Distance/SeparationDistance;
                Separation+=Away.GetSafeNormal2D()*Crowding;
                StrongestCrowding=FMath::Max(StrongestCrowding,Crowding);
            }
        }
        // Symmetric neighbors can cancel the radial vectors. A stable per-member side
        // preference gives crowded horses different ways around one another.
        if (StrongestCrowding>0.f)
        {
            const float Side=Horse->Brain->IndividualSteeringBias>=0.f?1.f:-1.f;
            Separation+=Horse->GetActorRightVector()*Side*StrongestCrowding*.65f;
        }
        Horse->Brain->SetHerdGuidance(HerdCenter,AverageVelocity,Separation,Neighbors);
    }
    AlarmSourceCount=Sources.Num();

    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : Members)
    {
        auto* Horse=HorsePtr.Get();
        if (Sources.Contains(Horse)) { AlarmTravelSeconds.Remove(Horse); continue; }
        float Closest=BIG_NUMBER;
        for (const auto* Source : Sources) { Closest=FMath::Min(Closest,FVector::Dist2D(Horse->GetActorLocation(),Source->GetActorLocation())); }
        if (Sources.IsEmpty()) { AlarmTravelSeconds.Remove(Horse); continue; }
        float& Travel=AlarmTravelSeconds.FindOrAdd(Horse);
        Travel+=Dt;
        if (Travel>=Closest/FMath::Max(1.f,AlarmPropagationSpeed))
        {
            Horse->Brain->ReceiveHerdAlarm(AlarmStrength,AlarmHoldSeconds);
            Travel=0.f;
        }
    }
}

void ASteppeHerdManager::EndPlay(const EEndPlayReason::Type Reason)
{
    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : Members)
    {
        if (auto* Horse=HorsePtr.Get()) { Horse->Destroy(); }
    }
    Members.Reset();
    AlarmTravelSeconds.Reset();
    Super::EndPlay(Reason);
}
