#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/HorseBrainComponent.h"
#include "AI/WildHorseConfig.h"
#include "AI/SteppeHerdManager.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"

namespace
{
    struct FWildTestWorld
    {
        UWorld* World;
        FWildTestWorld(bool bCreateFloor=true)
        {
            const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            if (bCreateFloor) { Block(FVector(0,0,-10),FVector(2000,2000,.2f)); }
        }
        AStaticMeshActor* Block(FVector Position, FVector Scale)
        {
            auto* Actor = World->SpawnActor<AStaticMeshActor>();
            auto* Mesh = Actor->GetStaticMeshComponent();
            Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Actor->SetActorScale3D(Scale);
            Actor->SetActorLocation(Position);
            return Actor;
        }
        void Begin()
        {
            World->InitializeActorsForPlay(FURL()); World->BeginPlay();
            World->GetWorldSettings()->NotifyBeginPlay(); World->GetWorldSettings()->NotifyMatchStarted();
        }
        void Step(float Seconds, int32 Hz=60)
        {
            for (int32 I=0; I<FMath::RoundToInt(Seconds*Hz); ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All,1.f/Hz); }
        }
        ~FWildTestWorld()
        {
            World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildPerceptionTest,"Steppe.P2.PerceptionAndState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildPerceptionTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Wild = Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Rider = Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(-2000,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    // Hold positions in this sensing test; the next test exercises real locomotion.
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->Brain->SetThreatTarget(Rider);
    Rider->GetCharacterMovement()->Velocity=FVector(180,0,0);
    Fixture.Step(.5f);
    TestTrue(TEXT("Clear line of sight detected"),Wild->Brain->bThreatVisible);
    TestEqual(TEXT("Slow approach has a calm observation period"),Wild->Brain->State,EWildHorseState::Roaming);
    Fixture.Step(1.5f);
    TestEqual(TEXT("Slow proximity first produces alertness"),Wild->Brain->State,EWildHorseState::Alert);
    Rider->GetCharacterMovement()->Velocity=FVector(1000,0,0);
    Fixture.Step(.2f);
    TestTrue(TEXT("Closing speed sensed"),Wild->Brain->ClosingSpeed>900);
    TestEqual(TEXT("Fast approach causes flight at same distance"),Wild->Brain->State,EWildHorseState::Fleeing);
    auto* Wall=Fixture.Block(FVector(-1000,0,200),FVector(1,20,4));
    Fixture.Step(.3f);
    TestFalse(TEXT("Wall blocks sight"),Wild->Brain->bThreatVisible);
    TestEqual(TEXT("Brief occlusion does not immediately cancel flight"),Wild->Brain->State,EWildHorseState::Fleeing);
    Fixture.Step(3.5f);
    TestEqual(TEXT("Lost threat moves into recovery"),Wild->Brain->State,EWildHorseState::Recovering);
    Fixture.Step(8.f);
    TestEqual(TEXT("Calms after cooldown"),Wild->Brain->State,EWildHorseState::Roaming);
    TestTrue(TEXT("Awareness remains bounded"),Wild->Brain->Awareness>=0 && Wild->Brain->Awareness<=1);
    Wall->Destroy(); Rider->GetCharacterMovement()->Velocity=FVector::ZeroVector;
    Rider->SetActorLocation(FVector(-250,0,100)); Fixture.Step(.2f);
    TestEqual(TEXT("Very close threat triggers immediate flight"),Wild->Brain->State,EWildHorseState::Fleeing);
    Rider->Destroy(); Fixture.Step(10.f);
    TestEqual(TEXT("Destroyed target safely releases threat"),Wild->Brain->State,EWildHorseState::Roaming);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildMovementTest,"Steppe.P2.MovementOwnershipAndObstacles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildMovementTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Wild = Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Rider = Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(-250,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Wild->Brain->SetThreatTarget(Rider);
    Fixture.Step(1.f);
    auto* Move=CastChecked<UHorseMovementComponent>(Wild->GetCharacterMovement());
    TestEqual(TEXT("Nearby rider triggers flight"),Wild->Brain->State,EWildHorseState::Fleeing);
    TestTrue(TEXT("AI intent accelerates through CMC rather than snapping speed"),Move->CurrentSpeed>20 && Move->CurrentSpeed<400);
    Fixture.Step(3.f);
    TestTrue(TEXT("Wild horse actually moves away"),Wild->GetActorLocation().X>500);
    TestTrue(TEXT("Rider intent was not used by AI"),Move->RiderIntent.IsNearlyZero());
    const FVector Place=Wild->GetActorLocation();
    Rider->SetActorLocation(Place+FVector(0,180,0));
    Move->StopMovementImmediately();
    TestFalse(TEXT("Wild horse cannot be mounted directly"),Rider->Riding->TryMount(Wild));
    // Surround with blocking geometry: no direction is clear, so AI requests braking.
    Fixture.Block(Place+FVector(180,0,0),FVector(.5f,6,4));
    Fixture.Block(Place+FVector(-180,0,0),FVector(.5f,6,4));
    Fixture.Block(Place+FVector(0,180,0),FVector(6,.5f,4));
    Fixture.Block(Place+FVector(0,-180,0),FVector(6,.5f,4));
    Fixture.Step(.3f);
    TestTrue(TEXT("Blocked steering is surfaced"),Wild->Brain->bPathBlocked);
    TestTrue(TEXT("Blocked horse requests braking"),Move->HorseIntent.BrakeStrength>.9f && Move->DesiredSpeed==0.f);
    TestTrue(TEXT("Fully blocked horse turns in place to search for an exit"),
        Wild->Brain->bRecoveringFromBlockage && FMath::Abs(Move->HorseIntent.DesiredTurn)>.9f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildHazardTest,"Steppe.P2.StoppingDistanceAndGaps",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildHazardTest::RunTest(const FString& Parameters)
{
    for (bool bGap : {false,true})
    {
        FWildTestWorld Fixture(!bGap);
        if (bGap)
        {
            // Far endpoint is supported, but a 300 cm gap interrupts the route.
            Fixture.Block(FVector(-500,0,-10),FVector(14,40,.2f));
            Fixture.Block(FVector(2000,0,-10),FVector(30,40,.2f));
        }
        else { Fixture.Block(FVector(1500,0,200),FVector(1,40,4)); }
        auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
        Fixture.Begin();
        auto* Move=CastChecked<UHorseMovementComponent>(Wild->GetCharacterMovement());
        Move->SetComponentTickEnabled(false);
        Move->SetMovementMode(MOVE_Walking);
        Move->Velocity=FVector(1200,0,0);
        Fixture.Step(.2f);
        TestTrue(TEXT("Probe includes braking distance and decision latency"),Wild->Brain->StoppingProbeDistance>1400.f);
        TestTrue(bGap?TEXT("Intermediate gap triggers hazard braking"):TEXT("Wall beyond old lookahead triggers early braking"),Wild->Brain->bBrakingForHazard);
        TestTrue(TEXT("Hazard emits emergency brake intent"),Move->HorseIntent.BrakeStrength>.9f && Move->HorseIntent.DesiredSpeed==0.f);
        Move->Velocity=FVector(0,1200,0);
        Fixture.Step(.2f);
        TestFalse(TEXT("Clear lateral ground releases hazard brake"),Wild->Brain->bBrakingForHazard);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildApproachTest,"Steppe.P2.SlowApproachAndRelease",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildApproachTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(-2000,0,100),FRotator::ZeroRotator);
    // Exercise the saved gameplay asset as well as native defaults used by other tests.
    Wild->Brain->Config=LoadObject<UWildHorseConfig>(nullptr,TEXT("/Game/Steppe/Data/Horses/DA_WildHorse_Default.DA_WildHorse_Default"));
    if (!TestNotNull(TEXT("Gameplay tuning asset loads"),Wild->Brain->Config.Get())) { return false; }
    Fixture.Begin();
    auto* Move=CastChecked<UHorseMovementComponent>(Wild->GetCharacterMovement());
    Move->SetComponentTickEnabled(false);
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->Brain->SetThreatTarget(Rider);
    // Hold distance constant to compare approaches without locomotion changing the stimulus.
    Rider->GetCharacterMovement()->Velocity=FVector(180,0,0);
    Fixture.Step(12.f);
    TestEqual(TEXT("Sustained slow pressure observes rather than panics"),Wild->Brain->State,EWildHorseState::Alert);
    TestTrue(TEXT("Slow awareness stays below flight threshold"),Wild->Brain->Awareness<Wild->Brain->GetConfig().FlightThreshold);
    Rider->SetActorLocation(FVector(-800,0,100)); Fixture.Step(1.f);
    TestEqual(TEXT("Slow approach at eight metres yields instead of fleeing"),Wild->Brain->State,EWildHorseState::Yielding);
    TestTrue(TEXT("Yield uses walking intent"),Move->HorseIntent.DesiredSpeed>0 && Move->HorseIntent.DesiredSpeed<=250);
    Rider->GetCharacterMovement()->Velocity=FVector::ZeroVector; Fixture.Step(1.f);
    TestEqual(TEXT("Stopping halts retreat while player remains visible"),Wild->Brain->State,EWildHorseState::Alert);
    TestEqual(TEXT("Observation requests no forward speed"),Move->HorseIntent.DesiredSpeed,0.f);
    Fixture.Step(8.f);
    TestEqual(TEXT("Visible stationary player permits full recovery"),Wild->Brain->State,EWildHorseState::Roaming);
    Rider->SetActorLocation(FVector(-2000,0,100));
    Rider->GetCharacterMovement()->Velocity=FVector(1000,0,0); Fixture.Step(.2f);
    TestEqual(TEXT("Fast approach flees at twenty metres"),Wild->Brain->State,EWildHorseState::Fleeing);
    Move->Velocity=FVector(1200,0,0); Fixture.Step(5.f);
    TestEqual(TEXT("Horse outrunning player does not falsely release pursuit"),Wild->Brain->State,EWildHorseState::Fleeing);
    Rider->GetCharacterMovement()->Velocity=FVector(-180,0,0); Fixture.Step(3.f);
    TestEqual(TEXT("Visible retreat releases flight"),Wild->Brain->State,EWildHorseState::Recovering);
    TestTrue(TEXT("Retreat is a signed approach speed"),Wild->Brain->ApproachSpeed<0.f);
    // Return to a slow close approach with real horse movement enabled.
    Move->Velocity=FVector::ZeroVector; Fixture.Step(8.f);
    Rider->SetActorLocation(FVector(-800,0,100)); Rider->GetCharacterMovement()->Velocity=FVector(180,0,0);
    Move->SetComponentTickEnabled(true); Fixture.Step(3.f);
    TestTrue(TEXT("Yield moves horse through CMC at a bounded walking speed"),Wild->GetActorLocation().X>30 && Move->CurrentSpeed<=250.f);
    TestTrue(TEXT("Slow physical approach never enters flight"),Wild->Brain->State!=EWildHorseState::Fleeing);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSmallHerdTest,"Steppe.P3.SmallHerdFormationAndAlarm",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSmallHerdTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(-2000,0,100),FRotator::ZeroRotator);
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(0,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=5;
    Herd->FormationSpacing=300.f;
    Herd->SeparationDistance=500.f;
    Herd->AlarmPropagationSpeed=900.f;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->SetThreatTarget(Rider);
    Herd->FinishSpawning(HerdTransform);
    Fixture.Begin();

    if (!TestEqual(TEXT("Manager spawns requested small herd"),Herd->Members.Num(),5)) { return false; }
    float MaximumFormationDistance=0.f;
    for (const auto& A : Herd->Members)
    {
        for (const auto& B : Herd->Members) { MaximumFormationDistance=FMath::Max(MaximumFormationDistance,FVector::Dist2D(A->GetActorLocation(),B->GetActorLocation())); }
    }
    TestTrue(TEXT("Formation has useful spatial extent"),MaximumFormationDistance>500.f);
    const FVector FirstGoal=(Herd->Members[0]->Brain->Goal-Herd->Members[0]->GetActorLocation()).GetSafeNormal2D();
    const FVector SecondGoal=(Herd->Members[1]->Brain->Goal-Herd->Members[1]->GetActorLocation()).GetSafeNormal2D();
    TestFalse(TEXT("Members choose distinct initial roam directions"),FirstGoal.Equals(SecondGoal,.02f));
    TestNotEqual(TEXT("Members have distinct reaction timing"),
        Herd->Members[0]->Brain->IndividualReactionScale,Herd->Members[1]->Brain->IndividualReactionScale);
    TestNotEqual(TEXT("Members have distinct steering bias"),
        Herd->Members[0]->Brain->IndividualSteeringBias,Herd->Members[1]->Brain->IndividualSteeringBias);

    TArray<FVector> StartLocations;
    for (const TObjectPtr<ASteppeWildHorseCharacter>& Member : Herd->Members) { StartLocations.Add(Member->GetActorLocation()); }
    Fixture.Step(4.f);
    int32 MovingMembers=0;
    for (int32 Index=0; Index<Herd->Members.Num(); ++Index)
    {
        MovingMembers+=FVector::Dist2D(StartLocations[Index],Herd->Members[Index]->GetActorLocation())>20.f;
    }
    TestTrue(TEXT("Independent pauses allow most members to begin roaming"),MovingMembers>=3);
    TestTrue(TEXT("Roaming herd keeps bodies from collapsing into one point"),Herd->MinimumMemberSpacing>100.f);

    const FVector PrimaryLocation=Herd->Members[0]->GetActorLocation();
    Rider->SetActorLocation(PrimaryLocation-FVector(2000,0,0));
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Rider->GetCharacterMovement()->Velocity=FVector(1000,0,0);
    for (int32 Index=0; Index<Herd->Members.Num(); ++Index)
    {
        Herd->Members[Index]->GetCharacterMovement()->SetComponentTickEnabled(false);
        if (Index>0) { Herd->Members[Index]->Brain->SetThreatTarget(nullptr); }
    }

    Fixture.Step(.2f);
    TestEqual(TEXT("Nearest directly threatened horse flees first"),Herd->Members[0]->Brain->State,EWildHorseState::Fleeing);
    int32 SecondaryFleeing=0;
    for (int32 Index=1; Index<Herd->Members.Num(); ++Index) { SecondaryFleeing+=Herd->Members[Index]->Brain->State==EWildHorseState::Fleeing; }
    TestEqual(TEXT("Alarm is not broadcast as an instantaneous group state"),SecondaryFleeing,0);

    Fixture.Step(.7f);
    int32 SecondaryAlert=0;
    for (int32 Index=1; Index<Herd->Members.Num(); ++Index)
    {
        SecondaryAlert+=Herd->Members[Index]->Brain->State==EWildHorseState::Alert;
    }
    TestTrue(TEXT("Nearby herd members receive a delayed warning"),SecondaryAlert>0);
    Fixture.Step(1.5f);
    SecondaryFleeing=0;
    for (int32 Index=1; Index<Herd->Members.Num(); ++Index)
    {
        SecondaryFleeing+=Herd->Members[Index]->Brain->State==EWildHorseState::Fleeing;
        TestTrue(TEXT("Manager supplies neighbor context"),Herd->Members[Index]->Brain->HerdNeighborCount>0);
    }
    TestEqual(TEXT("Warning eventually propagates through the small herd"),SecondaryFleeing,4);
    TestTrue(TEXT("Close formation produces separation guidance"),!Herd->Members[1]->Brain->HerdSeparation.IsNearlyZero());

    Herd->Members[0]->Destroy();
    Fixture.Step(.3f);
    TestEqual(TEXT("Manager removes destroyed members safely"),Herd->Members.Num(),4);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTargetIsolationTest,"Steppe.P4.TargetSelectionAndIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTargetIsolationTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(0,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=5;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->IsolationDistanceRequired=1500.f;
    Herd->IsolationHoldSeconds=1.f;
    Herd->FinishSpawning(HerdTransform);
    Fixture.Begin();
    if (!TestEqual(TEXT("Isolation test has a complete herd"),Herd->Members.Num(),5)) { return false; }

    Herd->Members[0]->SetActorLocation(FVector(2000,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    for (int32 Index=1; Index<Herd->Members.Num(); ++Index)
    {
        Herd->Members[Index]->SetActorLocation(FVector(0,(Index-2.5f)*300.f,100),false,nullptr,ETeleportType::TeleportPhysics);
    }
    for (const auto& Member : Herd->Members) { Member->GetCharacterMovement()->SetComponentTickEnabled(false); }
    auto* Selected=Herd->SelectFocusHorse(FVector(1000,0,100),FVector::ForwardVector);
    TestEqual(TEXT("View direction selects the intended horse"),Selected,Herd->Members[0].Get());
    TestTrue(TEXT("Selected brain drops herd direction pull"),Selected && Selected->Brain->bIsolationFocus);

    Fixture.Step(.6f);
    TestTrue(TEXT("Sustained separation advances progress"),Herd->IsolationProgress>0.f && Herd->IsolationProgress<1.f);
    const float ProgressBeforeReturn=Herd->IsolationProgress;
    Selected->SetActorLocation(FVector(400,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(.4f);
    TestTrue(TEXT("Returning toward the herd drains progress"),Herd->IsolationProgress<ProgressBeforeReturn);

    Selected->SetActorLocation(FVector(2200,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(1.2f);
    TestTrue(TEXT("Holding beyond the required distance completes isolation"),Herd->bTargetIsolated);
    TestEqual(TEXT("Completed isolation keeps a full progress value"),Herd->IsolationProgress,1.f);

    TestNull(TEXT("Pressing target on the same horse toggles focus off"),
        Herd->SelectFocusHorse(FVector(1000,0,100),FVector::ForwardVector));
    TestNull(TEXT("Focus clears from manager"),Herd->FocusedHorse.Get());
    TestFalse(TEXT("Focus clears from horse brain"),Selected->Brain->bIsolationFocus);
    Herd->SetFocusedHorse(Herd->Members[1]);
    Herd->Members[1]->Destroy();
    Fixture.Step(.3f);
    TestNull(TEXT("Destroyed target is released safely"),Herd->FocusedHorse.Get());
    return true;
}
#endif
