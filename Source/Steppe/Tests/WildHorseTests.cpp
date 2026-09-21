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
#include "Character/Rider/RiderBalanceComponent.h"
#include "Feedback/SteppeFeedbackComponent.h"
#include "Presentation/HorsePresentationComponent.h"
#include "Lasso/LassoComponent.h"
#include "Lasso/LassoTargetComponent.h"
#include "Core/SteppeGameplayTags.h"
#include "Game/SteppeTrialState.h"
#include "Game/SteppeGameMode.h"
#include "Playtest/SteppePlaytestMetrics.h"
#include "Capture/HorseTrustComponent.h"
#include "Camp/SteppeDeliveryZone.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Presentation/RiderAnimInstance.h"
#include "Presentation/HorseAnimInstance.h"
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildUnstuckTest,"Steppe.P2.ObstacleAndMountUnstuck",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildUnstuckTest::RunTest(const FString& Parameters)
{
    {
        FWildTestWorld Fixture;
        auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
        Fixture.Block(FVector(250,0,150),FVector(.5f,8.f,3.f));
        Fixture.Block(FVector(-450,0,150),FVector(.5f,8.f,3.f));
        Fixture.Block(FVector(0,250,150),FVector(8.f,.5f,3.f));
        Fixture.Block(FVector(0,-250,150),FVector(8.f,.5f,3.f));
        Fixture.Begin();
        Wild->Brain->Goal=FVector(1000,0,100);
        Fixture.Step(6.f);
        TestTrue(TEXT("Blocked horse backs away instead of only spinning"),Wild->GetActorLocation().X < -30.f);
    }
    {
        FWildTestWorld Fixture;
        auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
        auto* Mount=Fixture.World->SpawnActor<ASteppeHorseCharacter>(FVector(120,0,100),FRotator(0,180,0));
        auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(120,150,100),FRotator::ZeroRotator);
        Fixture.Begin();
        TestTrue(TEXT("Rider mounts stationary horse beside wild horse"),Rider->Riding->TryMount(Mount));
        Wild->Brain->SetCaptured(true);
        Wild->Brain->SetLeadTarget(Rider);
        const FVector Start=Wild->GetActorLocation();
        Fixture.Step(4.f);
        TestTrue(TEXT("Led wild horse navigates around a stationary mount"),
            FVector::Dist2D(Start,Wild->GetActorLocation())>80.f);
    }
    {
        FWildTestWorld Fixture;
        auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
        auto* Mount=Fixture.World->SpawnActor<ASteppeHorseCharacter>(FVector(120,0,100),FRotator::ZeroRotator);
        auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(120,150,100),FRotator::ZeroRotator);
        Fixture.Begin();
        TestTrue(TEXT("Rider mounts beside free wild horse"),Rider->Riding->TryMount(Mount));
        Wild->Brain->SetThreatTarget(Rider);
        const float InitialDistance=FVector::Dist2D(Wild->GetActorLocation(),Mount->GetActorLocation());
        Fixture.Step(4.f);
        TestTrue(TEXT("Free wild horse separates from stationary player mount"),
            FVector::Dist2D(Wild->GetActorLocation(),Mount->GetActorLocation())>InitialDistance+100.f);
    }
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
    TestEqual(TEXT("Playable scene defaults to a visible twelve-horse herd"),GetDefault<ASteppeGameMode>()->WildHorseCount,12);
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
    int32 CalmMembers=0;
    for (int32 Index=0; Index<Herd->Members.Num(); ++Index)
    {
        const auto* Member=Herd->Members[Index].Get();
        MovingMembers+=FVector::Dist2D(StartLocations[Index],Member->GetActorLocation())>10.f;
        if (Member->Brain->State==EWildHorseState::Roaming)
        {
            ++CalmMembers;
            const auto* Movement=Cast<UHorseMovementComponent>(Member->GetCharacterMovement());
            TestTrue(TEXT("Calm herd uses a walking target speed"),Movement->HorseIntent.DesiredSpeed<=115.1f);
            TestTrue(TEXT("Calm turn stays bounded outside obstacle recovery"),
                Member->Brain->bPathBlocked || Member->Brain->bRecoveringFromBlockage
                    || FMath::Abs(Movement->HorseIntent.DesiredTurn)<=.251f);
        }
    }
    TestTrue(TEXT("Calm members remain in roaming state"),CalmMembers>=3);
    TestTrue(TEXT("Independent pauses allow herd members to begin roaming"),MovingMembers>=2);
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
    TestTrue(TEXT("Manager supplies a common escape direction away from the rider"),
        FVector::DotProduct(Herd->HerdEscapeDirection,FVector::ForwardVector)>.8f);
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
    auto* Preview=Herd->FindFocusHorse(FVector(1000,0,100),FVector::ForwardVector);
    TestEqual(TEXT("Focus preview finds the intended horse"),Preview,Herd->Members[0].Get());
    TestNull(TEXT("Focus preview does not select the horse"),Herd->FocusedHorse.Get());
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
    auto* Observer=Fixture.World->SpawnActor<AActor>(AActor::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator);
    Herd->FocusLostDistance=1000.f;
    Herd->SetThreatTarget(Observer);
    Herd->SetFocusedHorse(Herd->Members[1]);
    Herd->Members[1]->SetActorLocation(FVector(2000,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(.3f);
    TestNull(TEXT("Focus automatically clears beyond its tracking range"),Herd->FocusedHorse.Get());
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

    TestTrue(TEXT("Non-isolated target can show aim feedback"),Rider->Lasso->BeginAimForTarget(Wild,false));
    TestEqual(TEXT("Early aim enters visible aiming state"),Rider->Lasso->State,ELassoState::Aiming);
    TestFalse(TEXT("Non-isolated target still cannot be thrown at"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Rider->Lasso->CancelAim();
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
    Fixture.Step(1.6f);
    TestEqual(TEXT("Recovery returns the lasso to storage"),Rider->Lasso->State,ELassoState::Stored);

    auto* Obstacle=Fixture.Block(FVector(0,800,170),FVector(2,.2f,3));
    TestTrue(TEXT("Lasso can aim before an obstructed throw"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Obstructed throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::RightVector));
    Fixture.Step(.4f);
    TestEqual(TEXT("World obstacle blocks the lasso"),Rider->Lasso->State,ELassoState::Recovering);
    TestTrue(TEXT("Blocked throw reports its cause"),Rider->Lasso->Feedback.Contains(TEXT("blocked")));
    Fixture.Step(1.6f);
    Obstacle->Destroy();

    TestTrue(TEXT("Lasso can be aimed again"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Missed throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::RightVector));
    Fixture.Step(1.f);
    TestEqual(TEXT("Out-of-range miss enters recovery"),Rider->Lasso->State,ELassoState::Recovering);
    Fixture.Step(1.6f);
    TestEqual(TEXT("Miss recovery completes"),Rider->Lasso->State,ELassoState::Stored);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLassoSkillTest,"Steppe.P12.SwingTimingAndHitZones",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLassoSkillTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1000,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);

    TestTrue(TEXT("Rushed throw can still be attempted"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Rushed throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::RightVector));
    const float RushedRadius=Rider->Lasso->EffectiveCaptureRadius;
    const float RushedRange=Rider->Lasso->EffectiveMaximumRange;
    TestTrue(TEXT("Rushed throw locks low stability"),Rider->Lasso->LastThrowStability<.3f);
    Fixture.Step(1.f);
    Fixture.Step(1.6f);

    TestTrue(TEXT("Lasso can prepare a stable swing"),Rider->Lasso->BeginAimForTarget(Wild,true));
    Fixture.Step(.6f);
    TestTrue(TEXT("Mid-cycle swing exposes a stable window"),Rider->Lasso->SwingStability>.9f);
    TestTrue(TEXT("Stable throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    TestTrue(TEXT("Stable throw opens a larger loop"),Rider->Lasso->EffectiveCaptureRadius>RushedRadius);
    TestTrue(TEXT("Stable throw reaches farther"),Rider->Lasso->EffectiveMaximumRange>RushedRange);
    const float LockedStability=Rider->Lasso->LastThrowStability;
    Fixture.Step(.5f);
    TestEqual(TEXT("Stable neck throw attaches"),Rider->Lasso->State,ELassoState::Attached);
    TestTrue(TEXT("Physical loop reports a configured horse target zone"),Rider->Lasso->HitZone!=ELassoHitZone::None);
    TestTrue(TEXT("Throw stability stays locked after release"),FMath::IsNearlyEqual(Rider->Lasso->LastThrowStability,LockedStability));

    TestEqual(TEXT("Greybox horse exposes four editable target volumes"),Wild->LassoTarget->Volumes.Num(),4);
    for (const FLassoTargetVolume& Volume : Wild->LassoTarget->Volumes)
    {
        TestEqual(TEXT("A volume center classifies as its configured zone"),
            Rider->Lasso->ClassifyHitZone(Wild,Wild->LassoTarget->GetVolumeCenter(Volume)),Volume.Zone);
    }
    Rider->Lasso->HitZone=ELassoHitZone::Neck;
    const float NeckSeconds=Rider->Lasso->GetEffectiveSubdueSeconds(Wild);
    Rider->Lasso->HitZone=ELassoHitZone::Head;
    const float HeadSeconds=Rider->Lasso->GetEffectiveSubdueSeconds(Wild);
    const float HeadTension=Rider->Lasso->GetHitZoneTensionMultiplier();
    Rider->Lasso->HitZone=ELassoHitZone::Torso;
    TestTrue(TEXT("Neck controls faster than head and torso"),NeckSeconds<HeadSeconds && NeckSeconds<Rider->Lasso->GetEffectiveSubdueSeconds(Wild));
    TestTrue(TEXT("Head amplifies tension more than torso"),HeadTension>Rider->Lasso->GetHitZoneTensionMultiplier());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicalLassoGeometryTest,"Steppe.P15.PhysicalLoopGeometryAndActualHit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPhysicalLassoGeometryTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Desired=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1400,500,100),FRotator::ZeroRotator);
    auto* Actual=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(900,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Desired->GetCharacterMovement()->SetComponentTickEnabled(false);
    Actual->GetCharacterMovement()->SetComponentTickEnabled(false);

    TestTrue(TEXT("Desired target starts the swing"),Rider->Lasso->BeginAimForTarget(Desired,true));
    TestTrue(TEXT("Physical loop can be thrown"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    const FVector InitialNormal=Rider->Lasso->SwingPlaneNormal;
    Fixture.Step(.1f);
    TestEqual(TEXT("Loop remains in flight before reaching the nearer horse"),Rider->Lasso->State,ELassoState::Thrown);
    TestTrue(TEXT("Gravity changes loop velocity and plane orientation"),Rider->Lasso->LoopVelocity.Z<Rider->Lasso->ThrowLift
        && !Rider->Lasso->SwingPlaneNormal.Equals(InitialNormal,.001f));
    TestTrue(TEXT("Loop radius opens during flight"),Rider->Lasso->LoopRadius>Rider->Lasso->MinimumLoopRadius);
    TestTrue(TEXT("Loop axes remain perpendicular to its physical plane"),
        FMath::Abs(FVector::DotProduct(Rider->Lasso->LoopAxisX,Rider->Lasso->SwingPlaneNormal))<.01f
        && FMath::Abs(FVector::DotProduct(Rider->Lasso->LoopAxisY,Rider->Lasso->SwingPlaneNormal))<.01f);
    Fixture.Step(.3f);
    TestEqual(TEXT("Physical loop attaches to the horse actually inside it"),Rider->Lasso->State,ELassoState::Attached);
    TestTrue(TEXT("Actual geometric hit replaces the Q desired target"),Rider->Lasso->Target.Get()==Actual);
    TestTrue(TEXT("Actual horse enters lassoed behavior"),Actual->Brain->bLassoed);
    TestFalse(TEXT("Q desired horse is not attached when the loop misses it"),Desired->Brain->bLassoed);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLassoTargetVolumesTest,"Steppe.P15.TargetVolumesFollowHorseAndRemainConfigurable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLassoTargetVolumesTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Horse=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(600,200,100),FRotator(0,90,0));
    Fixture.Begin();
    if (!TestNotNull(TEXT("Wild horse owns a target component"),Horse->LassoTarget.Get())) { return false; }
    auto* Target=Horse->LassoTarget.Get();
    const FLassoTargetVolume& Head=Target->Volumes.Last();
    const FVector Expected=Horse->GetActorTransform().TransformPosition(Head.LocalCenter);
    TestTrue(TEXT("Target volume follows the horse transform"),Target->GetVolumeCenter(Head).Equals(Expected,.01f));
    TestEqual(TEXT("Head center reports Head zone"),Target->ClassifyLocation(Expected),ELassoHitZone::Head);

    float Along=0.f;
    FVector Location;
    ELassoHitZone Zone=ELassoHitZone::None;
    const FVector PlaneNormal=Horse->GetActorRightVector();
    TestTrue(TEXT("Loop intersects a configured target volume"),Target->FindLoopIntersection(
        Expected-PlaneNormal*30.f,Expected+PlaneNormal*30.f,PlaneNormal,30.f,10.f,Along,Location,Zone));
    TestEqual(TEXT("Intersection returns the volume's gameplay zone"),Zone,ELassoHitZone::Head);
    TestTrue(TEXT("Intersection returns a bounded flight fraction"),Along>=0.f && Along<=1.f);

    Target->Volumes.Empty();
    TestFalse(TEXT("Removing all configured volumes makes the horse unhittable"),Target->FindLoopIntersection(
        Expected-PlaneNormal*30.f,Expected+PlaneNormal*30.f,PlaneNormal,30.f,10.f,Along,Location,Zone));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiderBalanceTest,"Steppe.P12.BalanceFallAndDraggedRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRiderBalanceTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Mount=Fixture.World->SpawnActor<ASteppeHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,180,100),FRotator::ZeroRotator);
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(0,1200,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    auto* MountMovement=Mount->GetCharacterMovement();
    MountMovement->SetComponentTickEnabled(false);

    const float ForwardLoad=Rider->Balance->CalculateLoad(.8f,.1f,1200.f,1.f,ELassoHitZone::Neck);
    const float SideLoad=Rider->Balance->CalculateLoad(.8f,1.f,1200.f,1.f,ELassoHitZone::Neck);
    const float StrongHeadLoad=Rider->Balance->CalculateLoad(.8f,1.f,1200.f,1.35f,ELassoHitZone::Head);
    const float TorsoLoad=Rider->Balance->CalculateLoad(.8f,1.f,1200.f,1.f,ELassoHitZone::Torso);
    TestTrue(TEXT("Side pull loads balance more than forward pull"),SideLoad>ForwardLoad*5.f);
    TestTrue(TEXT("Strong head catch is riskier than baseline neck"),StrongHeadLoad>SideLoad);
    TestTrue(TEXT("Torso catch reduces balance load"),TorsoLoad<SideLoad);

    TestTrue(TEXT("Balance fixture mounts safely"),Rider->Riding->TryMount(Mount));
    TestTrue(TEXT("Side target can be aimed"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Side target throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,190),(Wild->GetActorLocation()+FVector(0,0,60)-FVector(0,0,190)).GetSafeNormal()));
    Fixture.Step(.6f);
    if (!TestEqual(TEXT("Side target is attached"),Rider->Lasso->State,ELassoState::Attached)) { return false; }
    Rider->Balance->FallThreshold=.08f;
    Rider->Balance->WarningThreshold=.04f;
    Rider->Balance->BuildThreshold=0.f;
    MountMovement->Velocity=FVector(1200,0,0);
    Wild->SetActorLocation(Wild->GetActorLocation()+FVector(0,300,0),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(.25f);
    TestFalse(TEXT("Critical side load force-dismounts the rider"),Rider->Riding->IsMounted());
    TestEqual(TEXT("Nearby attached target starts a short drag"),Rider->Balance->State,ERiderBalanceState::Dragged);
    TestEqual(TEXT("Dragged state exposes its gameplay tag"),Rider->GetRiderStateTag(),SteppeTags::Rider_State_Dragged.GetTag());
    Rider->Lasso->Release();
    Fixture.Step(.05f);
    TestEqual(TEXT("Active rope release ends dragging"),Rider->Balance->State,ERiderBalanceState::Recovering);
    Fixture.Step(1.6f);
    TestEqual(TEXT("Fall recovery returns to stable"),Rider->Balance->State,ERiderBalanceState::Stable);
    TestTrue(TEXT("Rider survives and remains movable"),IsValid(Rider) && Rider->GetCharacterMovement()->MovementMode!=MOVE_None);

    MountMovement->Velocity=FVector::ZeroVector;
    Rider->SetActorLocation(Mount->GetActorLocation()+FVector(0,180,0),false,nullptr,ETeleportType::TeleportPhysics);
    Rider->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    TestTrue(TEXT("Recovered rider can remount"),Rider->Riding->TryMount(Mount));
    Wild->SetActorLocation(FVector(0,1200,100),false,nullptr,ETeleportType::TeleportPhysics);
    TestTrue(TEXT("Second side target can be aimed"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Second side throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,190),(Wild->GetActorLocation()+FVector(0,0,60)-FVector(0,0,190)).GetSafeNormal()));
    Fixture.Step(.6f);
    if (!TestEqual(TEXT("Second throw attaches"),Rider->Lasso->State,ELassoState::Attached)) { return false; }
    Rider->Balance->MaximumDraggedSeconds=.1f;
    MountMovement->Velocity=FVector(1200,0,0);
    Wild->SetActorLocation(Wild->GetActorLocation()+FVector(0,300,0),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(.18f);
    if (!TestEqual(TEXT("Second fall begins dragging"),Rider->Balance->State,ERiderBalanceState::Dragged)) { return false; }
    Fixture.Step(.2f);
    TestEqual(TEXT("Dragged rider regains their feet without dropping the rope"),Rider->Lasso->State,ELassoState::Attached);
    TestEqual(TEXT("Standing rider remains pulled by the running horse"),Rider->Balance->State,ERiderBalanceState::Pulled);
    TestEqual(TEXT("Pulled state exposes its gameplay tag"),Rider->GetRiderStateTag(),SteppeTags::Rider_State_Pulled.GetTag());
    Rider->Lasso->Release();
    Fixture.Step(.05f);
    TestEqual(TEXT("Player release ends the on-foot rope struggle"),Rider->Balance->State,ERiderBalanceState::Recovering);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFeedbackSignalsTest,"Steppe.P13.FeedbackSignalsAndEvents",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFeedbackSignalsTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Mount=Fixture.World->SpawnActor<ASteppeHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,180,100),FRotator::ZeroRotator);
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1100,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Rider->Feedback->bEnableProceduralFallback=false;
    TestTrue(TEXT("Sprint hoof cadence is faster than walking"),
        Rider->Feedback->GetHoofbeatInterval(EHorseGait::Sprint)<Rider->Feedback->GetHoofbeatInterval(EHorseGait::Walk));
    TestTrue(TEXT("Fatigued sprint breathing exceeds rested walking"),
        Rider->Feedback->CalculateBreathIntensity(1.f,.2f,EHorseGait::Sprint)>
        Rider->Feedback->CalculateBreathIntensity(.2f,1.f,EHorseGait::Walk));
    TestEqual(TEXT("Surface type 1 routes to grass"),Rider->Feedback->ResolveGroundSurface(SurfaceType1),ESteppeGroundSurface::Grass);
    TestEqual(TEXT("Surface type 2 routes to hard ground"),Rider->Feedback->ResolveGroundSurface(SurfaceType2),ESteppeGroundSurface::Hard);
    TestTrue(TEXT("Hard ground has a distinct cadence"),
        Rider->Feedback->GetSurfaceCadenceScale(ESteppeGroundSurface::Hard)<Rider->Feedback->GetSurfaceCadenceScale(ESteppeGroundSurface::Grass));

    TestTrue(TEXT("Feedback fixture mounts safely"),Rider->Riding->TryMount(Mount));
    auto* Move=CastChecked<UHorseMovementComponent>(Mount->GetCharacterMovement());
    Move->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    Move->CurrentSpeed=1200.f;
    Move->Gait=EHorseGait::Gallop;
    Mount->AnimationData.Gait=EHorseGait::Gallop;
    Mount->AnimationData.NormalizedSpeed=.8f;
    Mount->AnimationData.NormalizedAcceleration=.3f;
    Mount->AnimationData.LeanAmount=.5f;
    Mount->AnimationData.SlipAmount=.4f;
    Mount->Attributes->CurrentStamina=30.f;
    Fixture.Step(.65f);
    TestTrue(TEXT("Mounted gallop emits multiple hoofbeats"),Rider->Feedback->HoofbeatCount>=2);
    TestTrue(TEXT("Mounted speed drives wind feedback"),Rider->Feedback->WindIntensity>.1f);
    TestTrue(TEXT("Fatigue and speed drive breath feedback"),Rider->Feedback->BreathIntensity>.2f);
    TestEqual(TEXT("Prototype floor resolves to grass"),Rider->Feedback->GroundSurface,ESteppeGroundSurface::Grass);
    TestTrue(TEXT("Hoofbeats drive a dust pulse"),Rider->Feedback->DustPulse>0.f);
    TestTrue(TEXT("Presentation advances a continuous gait phase"),Mount->AnimationData.GaitPhase>0.f);
    TestTrue(TEXT("Presentation blends into the moving stride"),Mount->AnimationData.StrideBlend>.5f);
    TestTrue(TEXT("Horse lean becomes a readable body roll"),Mount->AnimationData.BodyRoll<0.f);
    TestTrue(TEXT("Skeletal horse mesh receives body dynamics without rotating gameplay root"),
        !Mount->GetMesh()->GetRelativeRotation().Equals(FRotator(0,-90.f,0),.1f)
        && FVector::DotProduct(Mount->GetActorUpVector(),FVector::UpVector)>.99f);
    auto* HorseAnim=CastChecked<UHorseAnimInstance>(Mount->GetMesh()->GetAnimInstance());
    Mount->AnimationData.bStruggling=true;
    Mount->AnimationData.ExternalForceAmount=.7f;
    Fixture.Step(.2f);
    TestEqual(TEXT("External rope load selects the temporary struggle animation"),
        HorseAnim->GetActiveSequence(),Mount->Presentation->TemporaryStruggleAnimation.Get());
    Mount->AnimationData.bStruggling=false;
    Mount->AnimationData.bStopping=true;
    Fixture.Step(.2f);
    TestEqual(TEXT("Low-speed braking selects the temporary settle animation"),
        HorseAnim->GetActiveSequence(),Mount->Presentation->TemporaryStopAnimation.Get());
    Mount->AnimationData.bStopping=false;
    TestTrue(TEXT("Rider exposes mounted presentation state"),Rider->PresentationData.bMounted);
    TestTrue(TEXT("Rider follows the horse lean"),Rider->PresentationData.BodyRoll<0.f);

    TestTrue(TEXT("Isolated target can enter feedback swing"),Rider->Lasso->BeginAimForTarget(Wild,true));
    Fixture.Step(.55f);
    const FVector Origin=Rider->GetActorLocation()+FVector(0,0,100);
    TestTrue(TEXT("Feedback throw starts"),Rider->Lasso->ThrowFrom(Origin,(Wild->GetActorLocation()+FVector(0,0,70)-Origin).GetSafeNormal()));
    Fixture.Step(.6f);
    TestEqual(TEXT("Feedback throw attaches"),Rider->Lasso->State,ELassoState::Attached);
    Rider->Lasso->Release();
    Fixture.Step(.05f);
    TestTrue(TEXT("Swing, throw, attach and release emit lasso events"),Rider->Feedback->LassoEventCount>=3);
    const int32 RiskBefore=Rider->Feedback->RiskEventCount;
    Rider->Feedback->EmitEvent(ESteppeFeedbackEvent::BalanceWarning);
    TestEqual(TEXT("Balance warning is counted as risk feedback"),Rider->Feedback->RiskEventCount,RiskBefore+1);
    TestTrue(TEXT("Disabling placeholder sound does not suppress signals"),Rider->Feedback->EventCount>Rider->Feedback->HoofbeatCount);
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
    Fixture.Step(Rider->Lasso->GetEffectiveSubdueSeconds(Wild)+.4f);
    TestEqual(TEXT("Steady useful tension subdues the horse"),Rider->Lasso->State,ELassoState::Subdued);
    TestEqual(TEXT("Subdued state exposes its gameplay tag"),Rider->Lasso->GetStateTag(),SteppeTags::Lasso_State_Subdued.GetTag());
    TestTrue(TEXT("Control progress completes"),Rider->Lasso->ControlProgress>=1.f);
    TestTrue(TEXT("Subdued horse remains lassoed"),Wild->Brain->bLassoed);

    Rider->Lasso->Release();
    Fixture.Step(1.6f);
    TestTrue(TEXT("A second rope fight can begin"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Second throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.5f);
    Rider->SetActorLocation(FVector(-2000,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Fixture.Step(.6f);
    TestEqual(TEXT("Sustained high tension keeps the rope attached"),Rider->Lasso->State,ELassoState::Attached);
    TestTrue(TEXT("Sustained pull keeps the horse lassoed"),Wild->Brain->bLassoed);
    const auto* WildMove=CastChecked<UHorseMovementComponent>(Wild->GetCharacterMovement());
    TestTrue(TEXT("Taut rope supplies a bounded pull toward the holder"),WildMove->ExternalAcceleration.X<0.f
        && WildMove->ExternalAcceleration.Size2D()<=WildMove->MaximumExternalAcceleration+.1f);
    TestEqual(TEXT("On-foot holder is pulled instead of losing the rope"),Rider->Balance->State,ERiderBalanceState::Pulled);
    TestEqual(TEXT("Steady speed does not create a rope shock"),Rider->Lasso->CalculateShockLoad(1200.f,0.f,1.2f),0.f);
    TestTrue(TEXT("High separating speed plus sudden deceleration creates a break-risk shock"),
        Rider->Lasso->CalculateShockLoad(1200.f,2000.f,1.2f)>Rider->Lasso->ShockBreakThreshold);
    Rider->Lasso->Release();
    TestTrue(TEXT("Releasing the rope removes its movement force"),WildMove->ExternalAcceleration.IsNearlyZero());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRopeRatchetAndWrapTest,"Steppe.P14.RopeRatchetAndObstacleWrap",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRopeRatchetAndWrapTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Wild=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1000,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    TestTrue(TEXT("Ratchet fixture aims"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("Ratchet fixture throws"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.5f);
    if (!TestEqual(TEXT("Ratchet fixture attaches"),Rider->Lasso->State,ELassoState::Attached)) { return false; }

    Wild->Brain->SetLassoConstraint(Rider->GetActorLocation(),.7f,true,.35f,false);
    const float TightenedLimit=Wild->Brain->LassoSpeedLimitScale;
    Wild->Brain->SetLassoConstraint(Rider->GetActorLocation(),.05f,false,0.f,false);
    TestEqual(TEXT("Released tension cannot restore a previously tightened speed limit"),Wild->Brain->LassoSpeedLimitScale,TightenedLimit);

    auto* Tree=Fixture.Block(FVector(500,20,150),FVector(.5f,.5f,2.5f));
    Rider->Lasso->SetBracing(true);
    Fixture.Step(.25f);
    TestTrue(TEXT("Obstacle between rider and horse creates one rope bend"),Rider->Lasso->bRopeWrapped);
    TestTrue(TEXT("Obstacle bend further limits the horse"),Wild->Brain->LassoSpeedLimitScale<TightenedLimit);
    TestTrue(TEXT("Obstacle bend adds rope resistance"),Rider->Lasso->Tension>=Rider->Lasso->ObstacleWrapTensionBonus);
    Tree->Destroy();
    Fixture.Step(.5f);
    TestFalse(TEXT("Clear line releases the temporary obstacle bend"),Rider->Lasso->bRopeWrapped);
    Rider->Lasso->Release();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnFootSurrenderTest,"Steppe.P14.OnFootSurrenderStartsLeading",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOnFootSurrenderTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    const FTransform HerdTransform(FRotator::ZeroRotator,FVector(700,0,100));
    auto* Herd=Fixture.World->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),HerdTransform,
        nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Herd->HerdSize=1;
    Herd->HorseClass=ASteppeWildHorseCharacter::StaticClass();
    Herd->SetThreatTarget(Rider);
    Herd->FinishSpawning(HerdTransform);
    Fixture.Begin();
    if (!TestEqual(TEXT("Surrender fixture has one horse"),Herd->Members.Num(),1)) { return false; }
    auto* Wild=Herd->Members[0].Get();
    Wild->SetActorLocation(FVector(700,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Wild->GetCharacterMovement()->SetComponentTickEnabled(false);
    Rider->GetCharacterMovement()->SetComponentTickEnabled(false);
    TestTrue(TEXT("On-foot surrender target can be aimed"),Rider->Lasso->BeginAimForTarget(Wild,true));
    TestTrue(TEXT("On-foot surrender throw starts"),Rider->Lasso->ThrowFrom(FVector(0,0,170),FVector::ForwardVector));
    Fixture.Step(.4f);
    if (!TestEqual(TEXT("On-foot surrender starts attached"),Rider->Lasso->State,ELassoState::Attached)) { return false; }
    Rider->SetActorLocation(FVector(500,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Rider->Lasso->OnFootSurrenderSeconds=.3f;
    Rider->Lasso->SetBracing(true);
    Fixture.Step(.4f);
    TestTrue(TEXT("Close steady on-foot hold fills surrender progress"),Rider->Lasso->OnFootSurrenderProgress>=1.f);
    TestTrue(TEXT("Completed surrender registers capture and lead"),Rider->Lasso->CompleteOnFootSurrender(Herd));
    TestEqual(TEXT("Surrender leaves the lasso secured"),Rider->Lasso->State,ELassoState::Captured);
    TestEqual(TEXT("Surrender removes the horse from the active herd"),Herd->Members.Num(),0);
    TestEqual(TEXT("Surrender counts first contact"),Herd->FirstContactCount,1);
    TestEqual(TEXT("Surrender immediately begins leading"),Wild->Trust->State,EPostCaptureState::Leading);
    TestTrue(TEXT("Surrendered horse follows the rope holder"),Wild->Brain->bCaptured && Wild->Brain->bLeading);
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
    Fixture.Step(1.6f);
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
    TestEqual(TEXT("First contact immediately begins leading"),Wild->Trust->State,EPostCaptureState::Leading);
    TestTrue(TEXT("Rider holds the lead horse"),Rider->Riding->GetLeadingHorse()==Wild);
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
    TestTrue(TEXT("Rider holds lead rope on foot"),Rider->Riding->GetLeadingHorse()==Wild);

    Herd->Tick(.2f);
    TestEqual(TEXT("Horse alone outside camp is not delivered"),Herd->DeliveredCount,0);
    auto* Mount=Fixture.World->SpawnActor<ASteppeHorseCharacter>(FVector(0,150,100),FRotator::ZeroRotator);
    TestFalse(TEXT("Leading horse does not consume mount interaction"),Herd->HandleFirstContactInteraction(Rider));
    TestTrue(TEXT("Rider can mount while holding lead rope"),Rider->Riding->TryMount(Mount));
    FRidingIntent SlowRide;
    SlowRide.Forward=1.f;
    SlowRide.bSprint=true;
    Rider->Riding->SetIntent(SlowRide);
    Rider->Riding->TickComponent(.1f,LEVELTICK_All,nullptr);
    const auto* MountMovement=Cast<UHorseMovementComponent>(Mount->GetCharacterMovement());
    TestTrue(TEXT("Mounted leading limits forward intent"),MountMovement->RiderIntent.Forward<=Rider->Riding->LeadRidingMaxForward+.001f);
    TestFalse(TEXT("Mounted leading prevents sprint"),MountMovement->RiderIntent.bSprint);
    Wild->Trust->TickComponent(.1f,LEVELTICK_All,nullptr);
    TestTrue(TEXT("Mounting preserves lead state"),Wild->Trust->State==EPostCaptureState::Leading && Wild->Brain->bLeading);
    Wild->SetActorLocation(FVector(2200,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Wild->Brain->TickComponent(.1f,LEVELTICK_All,nullptr);
    const auto* WildMovement=Cast<UHorseMovementComponent>(Wild->GetCharacterMovement());
    TestTrue(TEXT("Captured horse catches up beyond the old lead distance limit"),
        Wild->Brain->LeadDistance>Wild->Brain->LeadMaxDistance && WildMovement->HorseIntent.DesiredSpeed>0.f);
    Wild->SetActorLocation(FVector(100,0,100),false,nullptr,ETeleportType::TeleportPhysics);
    Herd->Tick(.2f);
    TestEqual(TEXT("Mounted rider and horse together in camp deliver once"),Herd->DeliveredCount,1);
    TestEqual(TEXT("Delivery stops lead intent"),Wild->Trust->State,EPostCaptureState::Delivered);
    TestTrue(TEXT("Delivery releases lead rope"),Rider->Riding->GetLeadingHorse()==nullptr);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlaytestMetricsTest,"Steppe.P14.PlaytestMetricTransitions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlaytestMetricsTest::RunTest(const FString& Parameters)
{
    FSteppePlaytestRound Round;
    Round.RecordLassoTransition(ELassoState::Aiming,ELassoState::Thrown,TEXT("in flight"),0.f,ELassoHitZone::None);
    Round.RecordLassoTransition(ELassoState::Thrown,ELassoState::Attached,TEXT("neck loop"),.62f,ELassoHitZone::Neck);
    Round.RecordLassoTransition(ELassoState::Attached,ELassoState::Recovering,TEXT("Rope broke - recovering"),.97f,ELassoHitZone::Neck);
    Round.RecordLassoTransition(ELassoState::Thrown,ELassoState::Recovering,TEXT("Missed - recovering"),.2f,ELassoHitZone::None);
    Round.RecordBalanceTransition(ERiderBalanceState::Stable,ERiderBalanceState::Warning,.6f,1.f);
    Round.RecordBalanceTransition(ERiderBalanceState::Warning,ERiderBalanceState::Falling,1.f,1.f);
    Round.RecordBalanceTransition(ERiderBalanceState::Falling,ERiderBalanceState::Dragged,1.f,1.f);
    TestEqual(TEXT("One throw is counted across flight and attachment"),Round.ThrowCount,1);
    TestEqual(TEXT("Attachment and hit zone are retained"),Round.AttachCount,1);
    TestEqual(TEXT("Attached hit zone is Neck"),Round.HitZone,FString(TEXT("Neck")));
    TestEqual(TEXT("Rope break is separated from a miss"),Round.RopeBreakCount,1);
    TestEqual(TEXT("A later miss is classified separately"),Round.MissCount,1);
    TestEqual(TEXT("Balance warning entry is counted"),Round.BalanceWarningCount,1);
    TestEqual(TEXT("Fall entry is counted"),Round.FallCount,1);
    TestEqual(TEXT("Dragged entry is counted"),Round.DraggedCount,1);
    TestTrue(TEXT("Peak rope tension is retained"),FMath::IsNearlyEqual(Round.PeakTension,.97f));
    TestTrue(TEXT("Peak normalized balance risk is retained"),FMath::IsNearlyEqual(Round.PeakBalanceRisk,1.f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiderPresentationSocketsTest,"Steppe.P16.RiderPresentationSockets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRiderPresentationSocketsTest::RunTest(const FString& Parameters)
{
    FWildTestWorld Fixture;
    auto* Rider=Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Horse=Fixture.World->SpawnActor<ASteppeHorseCharacter>(FVector(150,0,100),FRotator::ZeroRotator);
    auto* LassoTarget=Fixture.World->SpawnActor<ASteppeWildHorseCharacter>(FVector(1000,0,100),FRotator::ZeroRotator);
    Fixture.Begin();
    auto* RiderMesh=Rider->GetMesh()->GetSkeletalMeshAsset();
    auto* HorseMesh=Horse->GetMesh()->GetSkeletalMeshAsset();
    if (!TestNotNull(TEXT("Rider has a skeletal mannequin"),RiderMesh)
        || !TestNotNull(TEXT("Horse has a skeletal mesh"),HorseMesh)) { return false; }
    const USkeletalMeshSocket* Hand=RiderMesh->FindSocket(TEXT("LassoHand_R"));
    const USkeletalMeshSocket* Seat=HorseMesh->FindSocket(TEXT("RiderSeat"));
    if (!TestNotNull(TEXT("Rider has a lasso hand socket"),Hand)
        || !TestNotNull(TEXT("Horse has a rider seat socket"),Seat)) { return false; }
    TestEqual(TEXT("Lasso hand follows the right hand bone"),Hand->BoneName,FName(TEXT("hand_r")));
    TestEqual(TEXT("Rider seat follows the horse body bone"),Seat->BoneName,FName(TEXT("Body")));
    TestTrue(TEXT("Gameplay hand query uses the animated socket"),
        FVector::Dist(Rider->GetLassoHandLocation(),Rider->GetMesh()->GetSocketLocation(TEXT("LassoHand_R")))<1.f);
    TestTrue(TEXT("Rein hands resolve to separate skeletal contact points"),
        FVector::Dist(Rider->GetReinHandLocation(true),Rider->GetReinHandLocation(false))>8.f);
    TestTrue(TEXT("Rider mounts via the horse seat"),Rider->Riding->TryMount(Horse));
    Fixture.Step(.1f);
    TestNotNull(TEXT("Rider uses the blending animation instance"),
        Cast<URiderAnimInstance>(Rider->GetMesh()->GetAnimInstance()));
    auto CurrentRiderAnimation=[Rider]() -> FString
    {
        if (const auto* Anim=Cast<URiderAnimInstance>(Rider->GetMesh()->GetAnimInstance()))
        {
            return Anim->GetActiveSequence()?Anim->GetActiveSequence()->GetName():FString();
        }
        const auto* Instance=Rider->GetMesh()->GetSingleNodeInstance();
        return Instance && Instance->GetAnimationAsset()?Instance->GetAnimationAsset()->GetName():FString();
    };
    TestEqual(TEXT("Mounted rider uses the upright seated pose"),CurrentRiderAnimation(),FString(TEXT("RiderMounted_Pose")));
    TestEqual(TEXT("Mounted rider is attached to RiderSeat"),Rider->GetRootComponent()->GetAttachSocketName(),FName(TEXT("RiderSeat")));
    const FVector LeftFoot=Rider->GetFootLocation(true);
    const FVector RightFoot=Rider->GetFootLocation(false);
    TestTrue(TEXT("Mounted feet remain separated on opposite sides of the horse"),FVector::Dist(LeftFoot,RightFoot)>35.f);
    TestTrue(TEXT("Mounted feet sit below the rider seat"),LeftFoot.Z<Rider->GetActorLocation().Z-25.f && RightFoot.Z<Rider->GetActorLocation().Z-25.f);
    TestTrue(TEXT("Mounted rider remains upright despite imported bone frame"),
        FVector::DotProduct(Rider->GetActorUpVector(),FVector::UpVector)>.98f);
    TestTrue(TEXT("Mounted rider can begin the visible swing"),Rider->Lasso->BeginAimForTarget(LassoTarget,true));
    Fixture.Step(.05f);
    const FVector FirstHand=Rider->GetLassoHandLocation();
    Fixture.Step(.35f);
    TestTrue(TEXT("Gameplay swing phase moves the skeletal lasso hand"),
        FVector::Dist(FirstHand,Rider->GetLassoHandLocation())>2.f);
    Rider->Lasso->State=ELassoState::Thrown;
    Fixture.Step(.02f);
    TestEqual(TEXT("In-flight loop uses the mounted throw pose"),CurrentRiderAnimation(),FString(TEXT("RiderMountedThrow_Pose")));
    Rider->Lasso->State=ELassoState::Attached;
    Rider->Lasso->Target=LassoTarget;
    Rider->Lasso->bBracing=true;
    Rider->Lasso->Tension=.8f;
    LassoTarget->SetActorLocation(FVector(900,700,100));
    Fixture.Step(.25f);
    TestEqual(TEXT("Mounted rope control uses the seated brace pose"),CurrentRiderAnimation(),FString(TEXT("RiderMountedBrace_Pose")));
    TestTrue(TEXT("Rider turns toward a rope pulling from the right"),Rider->PresentationData.BodyYaw>8.f);
    TestTrue(TEXT("Rider leans back more under useful rope tension"),Rider->PresentationData.BodyPitch<-7.f);
    Rider->Lasso->bRopeWrapped=true;
    Rider->Lasso->RopeBendPoint=Rider->GetActorLocation()+Rider->GetActorForwardVector()*400.f-Rider->GetActorRightVector()*500.f;
    Fixture.Step(.35f);
    TestTrue(TEXT("Wrapped rope pose follows the first span around the obstacle"),Rider->PresentationData.BodyYaw<-8.f);
    Rider->Lasso->bRopeWrapped=false;
    Rider->Riding->Dismount();
    Fixture.Step(.02f);
    TestEqual(TEXT("On-foot rope control keeps a standing brace pose"),CurrentRiderAnimation(),FString(TEXT("RiderOnFootBrace_Pose")));
    return true;
}
#endif
