#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Character/Horse/HorseLocomotionMath.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Presentation/HorsePresentationComponent.h"
#include "Presentation/HorseAnimInstance.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSteppeGripMathTest,"Steppe.P16.GripResponse",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSteppeGripMathTest::RunTest(const FString& Parameters)
{
    const FVector Initial(1000.f,0.f,0.f);
    const FVector Facing(0.f,1.f,0.f);
    const FVector Loose=SteppeHorseMath::TurnVelocityTowardFacing(Initial,Facing,1000.f,3.5f,1.f/60);
    const FVector Firm=SteppeHorseMath::TurnVelocityTowardFacing(Initial,Facing,1000.f,18.f,1.f/60);
    TestTrue(TEXT("Higher grip aligns velocity faster"),Firm.Y>Loose.Y && Loose.X>Firm.X);
    TestTrue(TEXT("Turning preserves speed"),FMath::IsNearlyEqual(Loose.Size2D(),1000.f,.1f));
    TestTrue(TEXT("Zero grip preserves previous direction"),SteppeHorseMath::TurnVelocityTowardFacing(Initial,Facing,1000.f,0.f,1.f/60).Equals(Initial,.1f));
    TestTrue(TEXT("Stopped horse has no residual lateral velocity"),SteppeHorseMath::TurnVelocityTowardFacing(Initial,Facing,0.f,3.5f,1.f/60).IsNearlyZero());
    float YawAt30=0.f;
    float YawAt120=0.f;
    for (int32 Hz : {30,120})
    {
        FVector Velocity=Initial;
        for (int32 I=0;I<Hz;++I) { Velocity=SteppeHorseMath::TurnVelocityTowardFacing(Velocity,Facing,1000.f,3.5f,1.f/Hz); }
        if (Hz==30) { YawAt30=Velocity.Rotation().Yaw; } else { YawAt120=Velocity.Rotation().Yaw; }
    }
    TestTrue(TEXT("Grip response is nearly frame-rate independent"),FMath::Abs(YawAt30-YawAt120)<.1f);
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
    TestEqual(TEXT("Placeholder horse has a complete low-poly silhouette"),Horse->GetPlaceholderPartCount(),16);
    TestEqual(TEXT("Placeholder horse exposes four animated legs"),Horse->GetPlaceholderLegCount(),4);
    auto Step=[World](float Seconds)
    {
        // Engine tick tasks deduplicate on GFrameCounter, even across explicit World::Tick calls.
        for (int32 I=0; I<FMath::RoundToInt(Seconds*60); ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All,1.f/60); }
    };
    Step(.5f);
    TestTrue(TEXT("Temporary skeletal horse replaces visible placeholder"),Horse->GetMesh()->GetSkeletalMeshAsset()
        && !Horse->GetPlaceholderRoot()->IsVisible());
    TestTrue(TEXT("Horse begins in its animation blueprint idle pose"),
        Cast<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())
        && CastChecked<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())->ActiveGait==EHorseGait::Idle
        && CastChecked<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())->GetActiveSequence()==nullptr);
    auto* Move=CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement());
    Floor->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Steppe/Debug/M_PrototypeGrass.M_PrototypeGrass")));
    Step(.1f); const float GrassGrip=Move->EffectiveGripRate;
    Floor->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Steppe/Debug/M_PrototypeMarker.M_PrototypeMarker")));
    Step(.1f); const float HardGrip=Move->EffectiveGripRate;
    TestTrue(TEXT("Hard physical surface changes effective grip"),HardGrip>GrassGrip*1.04f);
    FRidingIntent Intent; Intent.Forward=1;
    Move->SetRiderIntent(Intent); Step(1);
    AddInfo(FString::Printf(TEXT("1s speed=%.1f desired=%.1f mode=%d location=%s begun=%d"),Move->CurrentSpeed,Move->DesiredSpeed,static_cast<int32>(Move->MovementMode),*Horse->GetActorLocation().ToString(),Horse->HasActorBegunPlay()));
    TestTrue(TEXT("Progressive acceleration, grounded"),Move->CurrentSpeed>50 && Move->CurrentSpeed<500 && Move->IsMovingOnGround());
    TestTrue(TEXT("Walking selects the imported walk animation"),Cast<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())
        && CastChecked<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())->GetActiveSequence()==Horse->Presentation->TemporaryWalkAnimation.Get());
    const int32 FrontLegBone=Horse->GetMesh()->GetBoneIndex(TEXT("frontupperleg_l"));
    TestTrue(TEXT("Imported front leg bone exists"),FrontLegBone!=INDEX_NONE);
    if (FrontLegBone!=INDEX_NONE)
    {
        const FQuat FirstPose=Horse->GetMesh()->GetBoneTransform(FrontLegBone).GetRotation();
        Step(.2f);
        const FQuat SecondPose=Horse->GetMesh()->GetBoneTransform(FrontLegBone).GetRotation();
        TestTrue(TEXT("Walking visibly rotates the skeletal front leg"),FirstPose.AngularDistance(SecondPose)>.03f);
    }
    Step(7); const float Gallop=Move->CurrentSpeed;
    AddInfo(FString::Printf(TEXT("8s speed=%.1f desired=%.1f location=%s"),Gallop,Move->DesiredSpeed,*Horse->GetActorLocation().ToString()));
    TestTrue(TEXT("Reaches gallop"),Gallop>1100);
    TestTrue(TEXT("Galloping selects the imported gallop animation"),Cast<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())
        && CastChecked<UHorseAnimInstance>(Horse->GetMesh()->GetAnimInstance())->GetActiveSequence()==Horse->Presentation->TemporaryGallopAnimation.Get());
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
    Direct.DesiredSpeed=1500; Move->SetHorseIntent(Direct); Step(.25f);
    TestTrue(TEXT("Facing and velocity separate during a turn"),FMath::Abs(Move->SlipAngleDegrees)>.1f && FMath::Abs(Move->LateralSpeed)>1.f);
    TestTrue(TEXT("Slip angle reaches horse presentation data"),FMath::Abs(Horse->AnimationData.SlipAmount)>.001f);
    Direct.DesiredTurn=0.f; Move->SetHorseIntent(Direct); Step(1.f);
    TestTrue(TEXT("Lateral slip settles after steering ends"),FMath::Abs(Move->SlipAngleDegrees)<1.f);
    Move->SetExternalAcceleration(Horse->GetActorRightVector()*2000.f);
    TestTrue(TEXT("External acceleration is bounded"),Move->ExternalAcceleration.Size2D()<=Move->MaximumExternalAcceleration+.1f);
    const float BeforePull=Move->LateralSpeed;
    Step(.2f);
    TestTrue(TEXT("Lateral pull changes the horse trajectory"),Move->LateralSpeed>BeforePull+10.f);
    Move->ClearExternalAcceleration();
    TestTrue(TEXT("External pull clears completely"),Move->ExternalAcceleration.IsNearlyZero());
    Direct.DesiredTurn=1.f; Move->SetHorseIntent(Direct); Step(10);
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
    auto* Obstacle=World->SpawnActor<AStaticMeshActor>();
    Obstacle->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Obstacle->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Obstacle->SetActorScale3D(FVector(.25f,1.5f,2.f));
    Obstacle->SetActorLocation(Horse->GetActorLocation()+Horse->GetActorForwardVector()*380.f);
    Obstacle->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Intent.Forward=1.f; Intent.Turn=0.f; Intent.bBrake=false; Intent.bSprint=false;
    Rider->Riding->SetIntent(Intent); Step(.12f);
    TestTrue(TEXT("Mounted horse detects an obstacle before collision"),Move->bRiderAvoidingObstacle);
    TestTrue(TEXT("Mounted horse adds a limited avoidance turn"),FMath::Abs(Move->RiderAvoidanceTurn)>.5f && FMath::Abs(Move->HorseIntent.DesiredTurn)<.8f);
    TestTrue(TEXT("Mounted horse reduces speed while steering around the obstacle"),Move->RiderAvoidanceSpeedScale<.81f);
    Obstacle->Destroy();
    Horse->Destroy(); TestFalse(TEXT("Destroyed mount releases rider"),Rider->Riding->IsMounted());
    World->EndPlay(EEndPlayReason::Quit);
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
