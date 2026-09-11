#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/HorseBrainComponent.h"
#include "AI/WildHorseConfig.h"
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
        FWildTestWorld()
        {
            const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            Block(FVector(0,0,-10),FVector(2000,2000,.2f));
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
    Rider->SetActorLocation(FVector(-500,0,100)); Fixture.Step(.2f);
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
    auto* Rider = Fixture.World->SpawnActor<ASteppeRiderCharacter>(FVector(-500,0,100),FRotator::ZeroRotator);
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
    return true;
}
#endif
