#include "AI/SteppeHerdManager.h"
#include "AI/HorseBrainComponent.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Capture/HorseTrustComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Camp/SteppeDeliveryZone.h"
#include "Player/SteppePlayerController.h"

ASteppeHerdManager::ASteppeHerdManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .2f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("HerdRoot")));
    HorseClass = ASteppeWildHorseCharacter::StaticClass();

    FWildHorseArchetypeProfile Fast;
    Fast.Archetype=EWildHorseArchetype::Fast; Fast.DisplayName=TEXT("Fast"); Fast.Temperament=TEXT("Keen"); Fast.Coat=TEXT("Golden Dun");
    Fast.DebugColor=FLinearColor(.72f,.46f,.12f); Fast.MaxSpeedMultiplier=1.18f; Fast.AccelerationMultiplier=1.12f;
    Fast.StaminaMultiplier=.9f; Fast.StrengthMultiplier=.85f; Fast.AgilityMultiplier=1.15f; Fast.FlightSpeedMultiplier=1.15f;
    Fast.StruggleMultiplier=1.05f; Fast.SubdueResistanceMultiplier=.85f;

    FWildHorseArchetypeProfile Strong;
    Strong.Archetype=EWildHorseArchetype::Strong; Strong.DisplayName=TEXT("Strong"); Strong.Temperament=TEXT("Steady"); Strong.Coat=TEXT("Dark Bay");
    Strong.DebugColor=FLinearColor(.24f,.09f,.035f); Strong.MaxSpeedMultiplier=.92f; Strong.AccelerationMultiplier=.9f;
    Strong.StaminaMultiplier=1.25f; Strong.StrengthMultiplier=1.35f; Strong.AgilityMultiplier=.85f;
    Strong.FearRiseMultiplier=.85f; Strong.FearDecayMultiplier=1.15f; Strong.FlightSpeedMultiplier=.9f;
    Strong.StruggleMultiplier=1.15f; Strong.SubdueResistanceMultiplier=1.35f; Strong.SafeApproachMultiplier=1.1f; Strong.CalmHoldMultiplier=.9f;

    FWildHorseArchetypeProfile Nervous;
    Nervous.Archetype=EWildHorseArchetype::Nervous; Nervous.DisplayName=TEXT("Nervous"); Nervous.Temperament=TEXT("Watchful"); Nervous.Coat=TEXT("Pale Grey");
    Nervous.DebugColor=FLinearColor(.62f,.68f,.72f); Nervous.MaxSpeedMultiplier=1.05f; Nervous.AccelerationMultiplier=1.08f;
    Nervous.StaminaMultiplier=.95f; Nervous.StrengthMultiplier=.9f; Nervous.AgilityMultiplier=1.1f;
    Nervous.FearRiseMultiplier=1.45f; Nervous.FearDecayMultiplier=.7f; Nervous.FlightSpeedMultiplier=1.08f;
    Nervous.StruggleMultiplier=1.1f; Nervous.SubdueResistanceMultiplier=1.05f; Nervous.SafeApproachMultiplier=.75f; Nervous.CalmHoldMultiplier=1.3f;

    ArchetypeProfiles={Fast,Strong,Nervous};
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
            Horse->Trust->InitializeIdentity(Index);
            if (!ArchetypeProfiles.IsEmpty()) { Horse->ApplyArchetype(ArchetypeProfiles[Index%ArchetypeProfiles.Num()]); }
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

ASteppeWildHorseCharacter* ASteppeHerdManager::FindFocusHorse(FVector ObserverLocation, FVector ViewDirection) const
{
    ViewDirection=ViewDirection.GetSafeNormal2D();
    ASteppeWildHorseCharacter* Best=nullptr;
    float BestScore=-BIG_NUMBER;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& MemberPtr : Members)
    {
        auto* Member=MemberPtr.Get();
        if (!IsValid(Member)) { continue; }
        const FVector ToMember=Member->GetActorLocation()-ObserverLocation;
        const float Distance=ToMember.Size2D();
        const float Facing=FVector::DotProduct(ViewDirection,ToMember.GetSafeNormal2D());
        if (Distance>FocusSelectionDistance || Facing<FocusSelectionMinDot) { continue; }
        const float Score=Facing*2.f-Distance/FMath::Max(1.f,FocusSelectionDistance);
        if (Score>BestScore) { BestScore=Score; Best=Member; }
    }
    return Best;
}

ASteppeWildHorseCharacter* ASteppeHerdManager::SelectFocusHorse(FVector ObserverLocation, FVector ViewDirection)
{
    ASteppeWildHorseCharacter* Best=FindFocusHorse(ObserverLocation,ViewDirection);
    if (Best==FocusedHorse) { ClearFocusedHorse(); return nullptr; }
    SetFocusedHorse(Best);
    return Best;
}

void ASteppeHerdManager::SetFocusedHorse(ASteppeWildHorseCharacter* Horse)
{
    if (IsValid(FocusedHorse)) { FocusedHorse->Brain->bIsolationFocus=false; }
    FocusedHorse=Members.Contains(Horse)?Horse:nullptr;
    if (FocusedHorse) { FocusedHorse->Brain->bIsolationFocus=true; }
    IsolationSeconds=0.f;
    IsolationProgress=0.f;
    IsolationDistance=0.f;
    bTargetIsolated=false;
}

void ASteppeHerdManager::ClearFocusedHorse()
{
    SetFocusedHorse(nullptr);
}

bool ASteppeHerdManager::RegisterCapturedHorse(ASteppeWildHorseCharacter* Horse)
{
    if (!IsValid(Horse) || !Members.Contains(Horse)) { return false; }
    if (FocusedHorse==Horse) { ClearFocusedHorse(); }
    Members.RemoveSingle(Horse);
    AlarmTravelSeconds.Remove(Horse);
    CapturedHorses.AddUnique(Horse);
    CapturedCount=CapturedHorses.Num();
    Horse->Brain->bIsolationFocus=false;
    Horse->Trust->BeginSecured(Cast<ASteppeRiderCharacter>(ThreatTarget.Get()));
    return true;
}

bool ASteppeHerdManager::HandleFirstContactInteraction(ASteppeRiderCharacter* Rider)
{
    if (!Rider) { return false; }
    ASteppeWildHorseCharacter* Closest=nullptr;
    float ClosestDistance=BIG_NUMBER;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : CapturedHorses)
    {
        auto* Horse=HorsePtr.Get();
        if (!IsValid(Horse) || !Horse->Trust || Horse->Trust->bNamed) { continue; }
        const float Distance=FVector::Dist2D(Horse->GetActorLocation(),Rider->GetActorLocation());
        if (Distance<=Horse->Trust->AwarenessRadius && Distance<ClosestDistance)
        {
            Closest=Horse;
            ClosestDistance=Distance;
        }
    }
    if (!Closest) { return false; }
    if (Closest->Trust->State==EPostCaptureState::FirstContact)
    {
        if (Closest->Trust->BeginLeading(Rider)) { LeadingHorse=Closest; }
    }
    else if (Closest->Trust->TryFirstContact(Rider))
    {
        FirstContactHorses.AddUnique(Closest);
        FirstContactCount=FirstContactHorses.Num();
    }
    // Consume E near a secured horse even when it is not ready, preventing an
    // unrelated mount attempt from hiding the approach feedback.
    return true;
}

void ASteppeHerdManager::SetDeliveryZone(ASteppeDeliveryZone* Zone) { DeliveryZone=Zone; }

bool ASteppeHerdManager::ConfirmDeliveredHorseName(AActor* HorseActor, const FString& NewName)
{
    auto* Horse=Cast<ASteppeWildHorseCharacter>(HorseActor);
    if (!Horse || !DeliveredHorses.Contains(Horse) || !Horse->Trust || !Horse->Trust->ConfirmName(NewName)) { return false; }
    NamedCount=0;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& Delivered : DeliveredHorses)
    {
        NamedCount+=Delivered && Delivered->Trust && Delivered->Trust->bNamed;
    }
    return true;
}

void ASteppeHerdManager::Tick(float Dt)
{
    Super::Tick(Dt);
    Members.RemoveAll([](const TObjectPtr<ASteppeWildHorseCharacter>& Horse) { return !IsValid(Horse); });
    if (FocusedHorse && !Members.Contains(FocusedHorse)) { ClearFocusedHorse(); }
    if (LeadingHorse && LeadingHorse->Trust && DeliveryZone)
    {
        auto* Rider=Cast<ASteppeRiderCharacter>(ThreatTarget.Get());
        if (Rider && DeliveryZone->ContainsActor(Rider) && DeliveryZone->ContainsActor(LeadingHorse)
            && LeadingHorse->Trust->MarkDelivered(Rider))
        {
            DeliveredHorses.AddUnique(LeadingHorse);
            DeliveredCount=DeliveredHorses.Num();
            if (auto* PC=Cast<ASteppePlayerController>(Rider->GetController())) { PC->ShowHorseNaming(LeadingHorse->Trust); }
            LeadingHorse=nullptr;
        }
    }
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

    RestHerdCenter=HerdCenter;
    IsolationDistance=0.f;
    if (FocusedHorse && Members.Num()>1)
    {
        RestHerdCenter=FVector::ZeroVector;
        int32 RestCount=0;
        for (const TObjectPtr<ASteppeWildHorseCharacter>& MemberPtr : Members)
        {
            if (MemberPtr.Get()!=FocusedHorse) { RestHerdCenter+=MemberPtr->GetActorLocation(); ++RestCount; }
        }
        if (RestCount>0) { RestHerdCenter/=RestCount; }
        IsolationDistance=FVector::Dist2D(FocusedHorse->GetActorLocation(),RestHerdCenter);
        if (!bTargetIsolated)
        {
            IsolationSeconds=IsolationDistance>=IsolationDistanceRequired?IsolationSeconds+Dt:FMath::Max(0.f,IsolationSeconds-Dt*2.f);
            IsolationProgress=FMath::Clamp(IsolationSeconds/FMath::Max(.1f,IsolationHoldSeconds),0.f,1.f);
            bTargetIsolated=IsolationProgress>=1.f;
        }
        else { IsolationProgress=1.f; }
    }

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
    ClearFocusedHorse();
    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : Members)
    {
        if (auto* Horse=HorsePtr.Get()) { Horse->Destroy(); }
    }
    for (const TObjectPtr<ASteppeWildHorseCharacter>& HorsePtr : CapturedHorses)
    {
        if (auto* Horse=HorsePtr.Get()) { Horse->Destroy(); }
    }
    Members.Reset();
    CapturedHorses.Reset();
    FirstContactHorses.Reset();
    FirstContactCount=0;
    LeadingHorse=nullptr;
    DeliveredHorses.Reset();
    DeliveredCount=0;
    NamedCount=0;
    DeliveryZone=nullptr;
    AlarmTravelSeconds.Reset();
    Super::EndPlay(Reason);
}
