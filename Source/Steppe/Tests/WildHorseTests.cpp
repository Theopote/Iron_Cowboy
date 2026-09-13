#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/HorseBrainComponent.h"
#include "AI/WildHorseConfig.h"
#include "AI/SteppeHerdManager.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Lasso/LassoComponent.h"
#include "Core/SteppeGameplayTags.h"
#include "Game/SteppeTrialState.h"
#include "Capture/HorseTrustComponent.h"
#include "Camp/SteppeDeliveryZone.h"
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLassoLoopTest,"Steppe.P5.LassoThrowAttachAndRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLassoLoopTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1000,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);

    TestFalse(TEXT("Non-isolated target cannot be aimed"),Rider->Lasso->BeginAimForTarget(Wild,false));
    TestEqual(TEXT("Rejected aim leaves lasso stored"),Rider->Lasso->State,ELassoState::Stored);
    TestTrue(TEXT("Isolated target enables aiming"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Aimed throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.5f);
    TestEqual(TEXT("Sweep attaches to the intended horse"),Rider->Lasso->State,ELassoState::Attached);
    TestTrue(TEXT("Attached state exposes its gameplay tag"),Rider->Lasso->GetStateTag()==SteppeTags::Lasso_State_Attached.GetTag());
    TestTrue(TEXT("Attached horse enters lassoed behavior"),Wild->Brain->bLassoed);
    TestEqual(TEXT("Lassoed behavior has a distinct state"),Wild->Brain->State,EWildHorseState::Lassoed);

    Rider->Lasso->Release();
    TestEqual(TEXT("Release starts recovery"),Rider->Lasso->State,ELassoState::Recovering);
    TestFalse(TEXT("Release frees the horse"),Wild->Brain->bLassoed);
    Fixture.Step(1.f);
    TestEqual(TEXT("Recovery returns the lasso to storage"),Rider->Lasso->State,ELassoState::Stored);

    auto* Obstacle=Fixture.Block(FVector(0,800,170),FVector(2,.2f,3));
    TestTrue(TEXT("Lasso can aim before an obstructed throw"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Obstructed throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::RightVector));
    Fixture.Step(.3f);
    TestEqual(TEXT("World obstacle blocks the lasso"),Rider->Lasso->State,ELassoState::Recovering);
    TestTrue(TEXT("Blocked throw reports its cause"),Rider->Lasso->Feedback.Contains(TEXT("blocked")));
    Fixture.Step(1.f);
    Obstacle->Destroy();

    TestTrue(TEXT("Lasso can be aimed again"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Missed throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::RightVector));
    Fixture.Step(1.f);
    TestEqual(TEXT("Out-of-range miss enters recovery"),Rider->Lasso->State,ELassoState::Recovering);
    Fixture.Step(1.f);
    TestEqual(TEXT("Miss recovery completes"),Rider->Lasso->State,ELassoState::Stored);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRopeFightTest,"Steppe.P6.RopeFightTensionAndSubdue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRopeFightTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1000,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);

    TestTrue(TEXT("Fight setup accepts isolated target"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Fight setup throws toward target"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.5f);
    if (!TestEqual(TEXT("Rope fight begins attached"),Rider->Lasso->State,ELassoState::Attached)) { return false; }
    Rider->Lasso->SetBracing(true);
    Fixture.Step(3.4f);
    TestEqual(TEXT("Steady useful tension subdues the horse"),Rider->Lasso->State,ELassoState::Subdued);
    TestEqual(TEXT("Subdued state exposes its gameplay tag"),Rider->Lasso->GetStateTag(),SteppeTags::Lasso_State_Subdued.GetTag());
    TestTrue(TEXT("Control progress completes"),Rider->Lasso->ControlProgress>=1.f);
    TestTrue(TEXT("Subdued horse remains lassoed"),Wild->Brain->bLassoed);

    Rider->Lasso->Release();
    Fixture.Step(1.f);
    TestTrue(TEXT("A second rope fight can begin"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Second throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.5f);
    Rider->SetActorLocation(FVector(-4000,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(.1f);
    TestEqual(TEXT("Excess rope length breaks the rope"),Rider->Lasso->State,ELassoState::Recovering);
    TestFalse(TEXT("Broken rope releases the horse"),Wild->Brain->bLassoed);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaptureTest,"Steppe.P7.CaptureSubduedHorse",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCaptureTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(1000,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=1;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->FinishSpawning(HerdTransform);
    Fixture.Begin();
    if (!TestEqual(TEXT("Capture fixture has one active horse"),Herd->Members.Num(),1)) { return false; }
    auto* Wild=Herd->Members[0].Get();
    Wild->SetActorLocation(FVector(1000,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    Herd->SetFocusedHorse(Wild);

    TestFalse(TEXT("Capture is rejected before subduing"),Rider->Lasso->CaptureWithHerd(Herd));
    TestTrue(TEXT("Capture setup aims"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Capture setup throws"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.5f);
    Rider->Lasso->SetBracing(true);
    Fixture.Step(3.4f);
    if (!TestEqual(TEXT("Capture prerequisite reaches Subdued"),Rider->Lasso->State,ELassoState::Subdued)) { return false; }

    TestTrue(TEXT("Subdued horse can be captured"),Rider->Lasso->CaptureWithHerd(Herd));
    TestEqual(TEXT("Lasso exposes captured result"),Rider->Lasso->State,ELassoState::Captured);
    TestEqual(TEXT("Captured lasso state exposes tag"),Rider->Lasso->GetStateTag(),SteppeTags::Lasso_State_Captured.GetTag());
    TestTrue(TEXT("Horse retains captured gameplay state"),Wild->Brain->bCaptured && Wild->Brain->State==EWildHorseState::Captured);
    TestEqual(TEXT("Horse exposes captured behavior tag"),Wild->Brain->GetBehaviorTag(),SteppeTags::Horse_State_Captured.GetTag());
    TestEqual(TEXT("Captured horse leaves active herd"),Herd->Members.Num(),0);
    TestEqual(TEXT("Capture result increments manager count"),Herd->CapturedCount,1);
    TestEqual(TEXT("Captured horse remains registered for result display"),Herd->CapturedHorses.Num(),1);
    TestNull(TEXT("Capture clears old focus"),Herd->FocusedHorse.Get());

    Rider->Lasso->Release();
    Fixture.Step(1.f);
    TestEqual(TEXT("Lasso can be stored after securing capture"),Rider->Lasso->State,ELassoState::Stored);
    TestTrue(TEXT("Stowing rope does not undo capture"),Wild->Brain->bCaptured);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerticalSliceRulesTest,"Steppe.P8.TimedMissionRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVerticalSliceRulesTest::RunTest(const FString& Parameters)
{
    FSteppeTrialProgress Trial;
    Trial.Start(10.f,2);
    TestEqual(TEXT("Mission starts running"),Trial.State,ESteppeTrialState::Running);
    TestEqual(TEXT("Mission stores its capture goal"),Trial.RequiredCaptures,2);

    Trial.Advance(3.f,1,0,0,0);
    TestEqual(TEXT("Partial capture keeps mission running"),Trial.State,ESteppeTrialState::Running);
    TestEqual(TEXT("Mission timer advances"),Trial.RemainingSeconds,7.f);

    Trial.Advance(2.f,2,2,2,1);
    TestEqual(TEXT("Delivery without all names keeps mission running"),Trial.State,ESteppeTrialState::Running);
    Trial.Advance(0.f,2,2,2,2);
    TestEqual(TEXT("Required names complete the mission"),Trial.State,ESteppeTrialState::Success);
    TestEqual(TEXT("Score combines captures and remaining time"),Trial.Score,2050);
    const float CompletionTime=Trial.RemainingSeconds;
    Trial.Advance(100.f,2,2,2,2);
    TestEqual(TEXT("Completed mission freezes its timer"),Trial.RemainingSeconds,CompletionTime);

    FSteppeTrialProgress Failed;
    Failed.Start(5.f,1);
    Failed.Advance(5.1f,0,0,0,0);
    TestEqual(TEXT("Expired mission fails without a capture"),Failed.State,ESteppeTrialState::Failed);
    TestEqual(TEXT("Failed mission has no score"),Failed.Score,0);

    FSteppeTrialProgress LastMoment;
    LastMoment.Start(1.f,1);
    LastMoment.Advance(1.f,1,1,1,1);
    TestEqual(TEXT("Naming on the final tick counts as success"),LastMoment.State,ESteppeTrialState::Success);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPostCaptureApproachTest,"Steppe.P9.PostCaptureApproachAndContact",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPostCaptureApproachTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(800,0,100),FRotator::ZeroRotator);
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(1000,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=1;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->SetThreatTarget(Rider);
    Herd->FinishSpawning(HerdTransform);
    Fixture.Begin();
    if (!TestEqual(TEXT("Approach fixture has one horse"),Herd->Members.Num(),1)) { return false; }
    auto* Wild=Herd->Members[0].Get();
    Wild->SetActorLocation(FVector(1000,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    TestTrue(TEXT("Secured horse registers with herd"),Herd->RegisterCapturedHorse(Wild));
    Wild->Brain->SetCaptured(true);
    TestEqual(TEXT("Capture begins secured approach state"),Wild->Trust->State,EPostCaptureState::Secured);

    Wild->Trust->AdvanceApproach(3.f,200.f,0.f,true);
    TestEqual(TEXT("Mounted rider cannot calm the horse"),Wild->Trust->State,EPostCaptureState::Secured);
    TestEqual(TEXT("Mounted rider gains no calm progress"),Wild->Trust->CalmProgress,0.f);

    Wild->Trust->AdvanceApproach(.1f,200.f,400.f,false);
    TestEqual(TEXT("Fast forward approach is rejected"),Wild->Trust->State,EPostCaptureState::Rejected);
    TestEqual(TEXT("Rejection clears calm progress"),Wild->Trust->CalmProgress,0.f);
    Wild->Trust->AdvanceApproach(1.6f,200.f,-500.f,false);
    TestTrue(TEXT("Fast retreat is not mistaken for a rush"),Wild->Trust->State!=EPostCaptureState::Rejected);
    Wild->Trust->AdvanceApproach(1.f,200.f,0.f,false);
    TestEqual(TEXT("Calm close hold enables contact"),Wild->Trust->State,EPostCaptureState::ReadyForContact);

    TestTrue(TEXT("Ready interaction is consumed"),Herd->HandleFirstContactInteraction(Rider));
    TestTrue(TEXT("First contact is recorded on horse"),Wild->Trust->bFirstContact);
    TestEqual(TEXT("First contact grants minimum trust"),Wild->Trust->Trust,Wild->Trust->FirstContactTrust);
    TestEqual(TEXT("Herd records first contact once"),Herd->FirstContactCount,1);
    TestTrue(TEXT("Horse remains captured after contact"),Wild->Brain->bCaptured && Herd->CapturedCount==1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeadDeliveryNamingTest,"Steppe.P10.LeadDeliveryAndNaming",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLeadDeliveryNamingTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(800,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=1;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->SetThreatTarget(Rider);
    Herd->FinishSpawning(HerdTransform);
    auto* Zone=Fixture.World->SpawnActor<ASteppeDeliveryZone>(FVector::ZeroVector,FRotator::ZeroRotator);
    Herd->SetDeliveryZone(Zone);
    Fixture.Begin();
    if (!TestEqual(TEXT("P10 fixture has one horse"),Herd->Members.Num(),1)) { return false; }
    auto* Wild=Herd->Members[0].Get();
    TestTrue(TEXT("Cannot lead before first contact"),!Wild->Trust->BeginLeading(Rider));
    TestTrue(TEXT("Horse registers captured"),Herd->RegisterCapturedHorse(Wild));
    Wild->Brain->SetCaptured(true);
    Wild->Trust->State=EPostCaptureState::FirstContact;
    Wild->Trust->bFirstContact=true;
    TestTrue(TEXT("First-contact interaction starts leading"),Herd->HandleFirstContactInteraction(Rider));
    TestEqual(TEXT("First-contact horse enters lead state"),Wild->Trust->State,EPostCaptureState::Leading);
    TestTrue(TEXT("Brain holds active lead target"),Wild->Brain->bLeading);

    Herd->Tick(.2f);
    TestEqual(TEXT("Horse alone outside camp is not delivered"),Herd->DeliveredCount,0);
    Wild->SetActorLocation(FVector(100,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Herd->Tick(.2f);
    TestEqual(TEXT("Rider and horse together in camp deliver once"),Herd->DeliveredCount,1);
    TestEqual(TEXT("Delivery stops lead intent"),Wild->Trust->State,EPostCaptureState::Delivered);
    TestTrue(TEXT("Empty horse name is rejected"),!Herd->ConfirmDeliveredHorseName(Wild,TEXT("   ")));
    TestTrue(TEXT("Valid horse name is accepted"),Herd->ConfirmDeliveredHorseName(Wild,TEXT("Saran")));
    TestEqual(TEXT("Horse stores trimmed name"),Wild->Trust->HorseName,FString(TEXT("Saran")));
    TestEqual(TEXT("Named result counts once"),Herd->NamedCount,1);
    TestTrue(TEXT("Second name cannot double count"),!Herd->ConfirmDeliveredHorseName(Wild,TEXT("Other")) && Herd->NamedCount==1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorseArchetypesTest,"Steppe.P11.ArchetypeGameplayDifferences",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorseArchetypesTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector::ZeroVector,FRotator::ZeroRotator);
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(1800,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=3;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->SetThreatTarget(Rider);
    Herd->FinishSpawning(HerdTransform);
    Fixture.Begin();
    if (!TestEqual(TEXT("Archetype fixture has three horses"),Herd->Members.Num(),3)) { return false; }
    auto* Fast=Herd->Members[0].Get();
    auto* Strong=Herd->Members[1].Get();
    auto* Nervous=Herd->Members[2].Get();
    TestEqual(TEXT("First profile is Fast"),Fast->Archetype,EWildHorseArchetype::Fast);
    TestEqual(TEXT("Second profile is Strong"),Strong->Archetype,EWildHorseArchetype::Strong);
    TestEqual(TEXT("Third profile is Nervous"),Nervous->Archetype,EWildHorseArchetype::Nervous);
    TestTrue(TEXT("Fast has higher top speed than Strong"),Fast->Attributes->MaxSpeed>Strong->Attributes->MaxSpeed);
    TestTrue(TEXT("Fast accelerates harder than Strong"),Fast->Attributes->Acceleration>Strong->Attributes->Acceleration);
    TestTrue(TEXT("Strong has more stamina and strength than Fast"),Strong->Attributes->MaxStamina>Fast->Attributes->MaxStamina
        && Strong->Attributes->Strength>Fast->Attributes->Strength);
    TestTrue(TEXT("Strong requires longer steady rope control"),Rider->Lasso->GetEffectiveSubdueSeconds(Strong)>Rider->Lasso->GetEffectiveSubdueSeconds(Fast));
    TestTrue(TEXT("Nervous fear rises faster and decays slower"),Nervous->Brain->AwarenessRiseScale>Fast->Brain->AwarenessRiseScale
        && Nervous->Brain->AwarenessDecayScale<Fast->Brain->AwarenessDecayScale);
    TestTrue(TEXT("Nervous approach window is stricter"),Nervous->Trust->SafeApproachSpeed<Fast->Trust->SafeApproachSpeed
        && Nervous->Trust->CalmHoldSeconds>Fast->Trust->CalmHoldSeconds);

    Fast->Trust->BeginSecured(Rider);
    Nervous->Trust->BeginSecured(Rider);
    Fast->Trust->AdvanceApproach(1.f,200.f,100.f,false);
    Nervous->Trust->AdvanceApproach(1.f,200.f,100.f,false);
    TestTrue(TEXT("Same approach calms Fast but not Nervous"),Fast->Trust->CalmProgress>0.f && Nervous->Trust->CalmProgress==0.f);
    const float FastSpeed=Fast->Attributes->MaxSpeed;
    Fast->ApplyArchetype(Herd->ArchetypeProfiles[0]);
    TestEqual(TEXT("Profile application is idempotent"),Fast->Attributes->MaxSpeed,FastSpeed);
    return true;
}
#endif
