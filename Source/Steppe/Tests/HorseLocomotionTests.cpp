#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Character/Horse/HorseLocomotionMath.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSteppeMathTest,"Steppe.P1.MathAndStamina",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSteppeMathTest::RunTest(const FString& Parameters)
{
    const auto& C=*GetDefault<UHorseLocomotionConfig>();
    TestEqual(TEXT("Acceleration has units cm/s2"),SteppeHorseMath::ApproachSpeed(0,1200,200,100,.5f),100.f);
    TestEqual(TEXT("Brake cannot reverse"),SteppeHorseMath::ApproachSpeed(50,0,200,600,1),0.f);
    TestEqual(TEXT("Cannot overshoot target"),SteppeHorseMath::ApproachSpeed(1100,1200,300,100,1),1200.f);
    for (int32 Hz : {30,60,120})
    {
        float Speed=0;
        for (int32 I=0; I<Hz*3; ++I) { Speed=SteppeHorseMath::ApproachSpeed(Speed,1200,200,100,1.f/Hz); }
        TestTrue(TEXT("3-second acceleration independent of frame rate"),FMath::IsNearlyEqual(Speed,600.f,.1f));
    }
    TestEqual(TEXT("Lower gait stable inside hysteresis"),SteppeHorseMath::SelectGait(295,EHorseGait::Walk,C),EHorseGait::Walk);
    TestEqual(TEXT("Upper gait stable inside hysteresis"),SteppeHorseMath::SelectGait(285,EHorseGait::Trot,C),EHorseGait::Trot);
    TestTrue(TEXT("High-speed yaw strongly limited"),C.SpeedTurnCurve.GetRichCurveConst()->Eval(1)<C.SpeedTurnCurve.GetRichCurveConst()->Eval(.1f)*.4f);
    auto* A=NewObject<UHorseAttributeComponent>();
    A->UpdateStamina(100,1); TestEqual(TEXT("Stamina clamps to zero"),A->CurrentStamina,0.f);
    A->UpdateStamina(100,-1); TestEqual(TEXT("Stamina clamps to max"),A->CurrentStamina,A->MaxStamina);
    TestEqual(TEXT("km/h conversion"),SteppeUnits::ToKilometersPerHour(1000),36.f);
    FRidingIntent Intent; Intent.Forward=100; Intent.Turn=-50; Intent.Clamp();
    TestEqual(TEXT("Forward clamps"),Intent.Forward,1.f); TestEqual(TEXT("Turn clamps"),Intent.Turn,-1.f);
    Intent.Reset(); TestTrue(TEXT("Intent reset"),Intent.IsNearlyZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSteppeWorldTest,"Steppe.P1.WorldMovementAndRiding",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSteppeWorldTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Floor=World->SpawnActor<AStaticMeshActor>();
    Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(2000,2000,.2f)); Floor->SetActorLocation(FVector(0,0,-10));
    Floor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    auto* Horse=World->SpawnActor<ASteppeHorseCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
    auto* Rider=World->SpawnActor<ASteppeRiderCharacter>(FVector(0,200,100),FRotator::ZeroRotator);
    World->InitializeActorsForPlay(FURL()); World->BeginPlay();
    World->GetWorldSettings()->NotifyBeginPlay();
    World->GetWorldSettings()->NotifyMatchStarted();
    auto Step=[World](float Seconds)
    {
        // Engine tick tasks deduplicate on GFrameCounter, even across explicit World::Tick calls.
        for (int32 I=0; I<FMath::RoundToInt(Seconds*60); ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All,1.f/60); }
    };
    Step(.5f);
    auto* Move=CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement());
    FRidingIntent Intent; Intent.Forward=1;
    Move->SetRiderIntent(Intent); Step(1);
    AddInfo(FString::Printf(TEXT("1s speed=%.1f desired=%.1f mode=%d location=%s begun=%d"),Move->CurrentSpeed,Move->DesiredSpeed,static_cast<int32>(Move->MovementMode),*Horse->GetActorLocation().ToString(),Horse->HasActorBegunPlay()));
    TestTrue(TEXT("Progressive acceleration, grounded"),Move->CurrentSpeed>50 && Move->CurrentSpeed<500 && Move->IsMovingOnGround());
    Step(7); const float Gallop=Move->CurrentSpeed;
    AddInfo(FString::Printf(TEXT("8s speed=%.1f desired=%.1f location=%s"),Gallop,Move->DesiredSpeed,*Horse->GetActorLocation().ToString()));
    TestTrue(TEXT("Reaches gallop"),Gallop>1100);
    Move->ClearIntent(); Step(1); const float Released=Move->CurrentSpeed;
    TestTrue(TEXT("Release preserves momentum"),Released>500 && Released<Gallop);
    Intent.bBrake=true; Move->SetRiderIntent(Intent); Step(1);
    TestTrue(TEXT("Brake stronger than coasting"),Released-Move->CurrentSpeed>2*(Gallop-Released));
    Step(3); TestTrue(TEXT("Brake stops without reverse"),Move->CurrentSpeed<1);
    Intent.bBrake=false; Intent.bSprint=true; Move->SetRiderIntent(Intent); Step(12);
    TestTrue(TEXT("Sprint drains stamina"),Horse->Attributes->CurrentStamina<90);
    Horse->Attributes->UpdateStamina(100,1); Step(.1f);
    TestTrue(TEXT("Exhaustion caps requested speed"),Move->DesiredSpeed<=1200);
    Intent.bBrake=true; Move->SetRiderIntent(Intent); Step(4);
    FHorseMovementIntent Direct; Direct.DesiredSpeed=180; Direct.DesiredTurn=1;
    Horse->Attributes->bInfiniteStamina=true;
    Move->SetHorseIntent(Direct); Step(3); const float LowTurnRate=Move->EffectiveTurnRate;
    Direct.DesiredSpeed=1500; Move->SetHorseIntent(Direct); Step(10);
    TestTrue(TEXT("Actual high-speed turn rate is smaller"),Move->EffectiveTurnRate<LowTurnRate*.4f);
    TestTrue(TEXT("Turn stress warns at sprint"),Move->TurnStress>.8f);
    Direct.DesiredSpeed=0; Direct.BrakeStrength=1; Move->SetHorseIntent(Direct); Step(4);
    Rider->SetActorLocation(Horse->GetActorLocation()+FVector(0,180,0));
    TestTrue(TEXT("Mount succeeds"),Rider->Riding->TryMount(Horse));
    Intent.bBrake=false; Intent.bSprint=false; Rider->Riding->SetIntent(Intent); Step(6);
    TestTrue(TEXT("Rider pipeline drives horse"),Move->CurrentSpeed>900);
    TestTrue(TEXT("Camera FOV increases"),Rider->Camera->FieldOfView>72);
    TestTrue(TEXT("Camera distance increases"),Rider->CameraBoom->TargetArmLength>390);
    Rider->Riding->Dismount(); TestTrue(TEXT("Unsafe high-speed dismount rejected"),Rider->Riding->IsMounted());
    Intent.Forward=0; Intent.bBrake=true; Rider->Riding->SetIntent(Intent); Step(4);
    Rider->Riding->Dismount(); TestFalse(TEXT("Safe dismount succeeds"),Rider->Riding->IsMounted());
    TestTrue(TEXT("Remount succeeds"),Rider->Riding->TryMount(Horse));
    Horse->Destroy(); TestFalse(TEXT("Destroyed mount releases rider"),Rider->Riding->IsMounted());
    World->EndPlay(EEndPlayReason::Quit);
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
