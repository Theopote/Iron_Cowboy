#include "Game/SteppeGameMode.h"
#include "Player/SteppePlayerController.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/RiderBalanceComponent.h"
#include "Debug/SteppeHUD.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Steppe.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "AI/HorseBrainComponent.h"
#include "AI/WildHorseConfig.h"
#include "AI/SteppeHerdManager.h"
#include "Lasso/LassoComponent.h"
#include "Capture/HorseTrustComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camp/SteppeDeliveryZone.h"
#include "UI/HorseNamingWidget.h"
#include "Feedback/SteppeFeedbackComponent.h"
#include "Playtest/SteppePlaytestMetrics.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Presentation/HorseAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "World/SteppeTerrainZone.h"
#include "World/SteppeGrassField.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
ASteppeGameMode::ASteppeGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    PlayerControllerClass = ASteppePlayerController::StaticClass();
    DefaultPawnClass=ASteppeRiderCharacter::StaticClass();
    HUDClass=ASteppeHUD::StaticClass();
    HorseClass=ASteppeHorseCharacter::StaticClass();
    WildHorseClass=ASteppeWildHorseCharacter::StaticClass();
    DeliveryZoneClass=ASteppeDeliveryZone::StaticClass();
    PlaytestMetrics=CreateDefaultSubobject<USteppePlaytestMetricsComponent>(TEXT("PlaytestMetrics"));
}
void ASteppeGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bEnableTrial && Trial.State==ESteppeTrialState::Running)
    {
        Trial.Advance(DeltaSeconds,HerdManager?HerdManager->CapturedCount:0,HerdManager?HerdManager->FirstContactCount:0,
            HerdManager?HerdManager->DeliveredCount:0,HerdManager?HerdManager->NamedCount:0);
    }
    if (PlaytestMetrics) { PlaytestMetrics->Observe(Trial); }
}
void ASteppeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
    auto* Rider=Cast<ASteppeRiderCharacter>(NewPlayer->GetPawn()); if (!Rider) { return; }
    if (!IsValid(DeliveryZone))
    {
        DeliveryZone=GetWorld()->SpawnActor<ASteppeDeliveryZone>(DeliveryZoneClass,DeliveryZoneTransform);
    }
    if (!IsValid(PlaygroundHorse))
    {
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        PlaygroundHorse=GetWorld()->SpawnActor<ASteppeHorseCharacter>(HorseClass,HorseSpawnTransform,Params);
    }
    if (PlaygroundHorse)
    {
        PlaygroundHorse->Attributes->bInfiniteStamina=bInfiniteStamina;
        if (bStartMounted) { Rider->Riding->TryMount(PlaygroundHorse); }
    }
    else { UE_LOG(LogSteppe,Error,TEXT("Playground horse could not spawn; check spawn transform/collision.")); }
    if (bSpawnWildHorse)
    {
        if (!IsValid(HerdManager))
        {
            HerdManager=GetWorld()->SpawnActorDeferred<ASteppeHerdManager>(ASteppeHerdManager::StaticClass(),WildHorseSpawnTransform,
                nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (HerdManager)
            {
                HerdManager->HorseClass=WildHorseClass;
                HerdManager->HerdSize=WildHorseCount;
                HerdManager->HabitatExtents=HerdHabitatExtents;
                HerdManager->SetThreatTarget(Rider);
                HerdManager->FinishSpawning(WildHorseSpawnTransform);
                HerdManager->EnsureMembersSpawned();
            }
        }
        if (HerdManager)
        {
            HerdManager->SetThreatTarget(Rider);
            HerdManager->SetDeliveryZone(DeliveryZone);
            WildHorses=HerdManager->Members;
            WildHorse=WildHorses.IsEmpty()?nullptr:WildHorses[0];
        }
        if (!WildHorse) { UE_LOG(LogSteppe, Error, TEXT("Wild herd spawn failed; check WildHorseClass and spawn clearance.")); }
    }
    if (bDebugEnabled) { IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse"))->Set(1,ECVF_SetByCode); }
    const bool bVerticalFailureSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeVerticalFailureSmoke"));
    if (bEnableTrial && Trial.State==ESteppeTrialState::NotStarted)
    {
        Trial.Start(bVerticalFailureSmoke?3.f:TrialDurationSeconds,RequiredCaptures);
    }
    if (PlaytestMetrics) { PlaytestMetrics->BeginRound(Rider,HerdManager,Trial); }
    if (FParse::Param(FCommandLine::Get(),TEXT("SteppeSmoke")))
    {
        // Explicit development smoke mode; normal play never injects input or exits.
        FApp::SetUseFixedTimeStep(true);
        FApp::SetFixedDeltaTime(1.0/60.0);
        NewPlayer->SetControlRotation(FRotator(-12,25,0));
        const bool bHerdIdleSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeHerdIdleSmoke"));
        const bool bIsolationSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeIsolationSmoke"));
        const bool bLassoSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeLassoSmoke"));
        const bool bRopeFightSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeRopeFightSmoke"));
        const bool bCaptureSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeCaptureSmoke"));
        const bool bVerticalSliceSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeVerticalSliceSmoke"));
        const bool bPostCaptureSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppePostCaptureSmoke"));
        const bool bFullLoopSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeFullLoopSmoke"));
        const bool bArchetypeSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeArchetypeSmoke"));
        const bool bLassoSkillSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeLassoSkillSmoke"));
        const bool bBalanceSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeBalanceSmoke"));
        const bool bFeedbackSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeFeedbackSmoke"));
        const bool bPresentationSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppePresentationSmoke"));
        const bool bGrasslandSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeGrasslandSmoke"));
        const bool bGrasslandTraversalSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeGrasslandTraversalSmoke"));
        const bool bMetricsSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeMetricsSmoke"));
        const bool bMetricsFailureSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeMetricsFailureSmoke"));
        const bool bHorseModelSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeHorseModelSmoke"));
        const bool bTimedLassoSmoke=bLassoSkillSmoke || bBalanceSmoke || bFeedbackSmoke || bFullLoopSmoke;
        const bool bFullLoopSequence=bVerticalSliceSmoke || bFullLoopSmoke;
        const bool bPostCaptureSequence=bVerticalSliceSmoke || bPostCaptureSmoke || bFullLoopSmoke;
        // Process-local count is restricted to this opt-in standalone smoke run.
        static int32 RetrySmokeCount=0;
        if (FParse::Param(FCommandLine::Get(),TEXT("SteppeRetrySmoke")))
        {
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_RETRY_SMOKE: Reload=%d Mounted=%d Awareness=%.2f"),
                RetrySmokeCount,Rider->Riding->IsMounted(),WildHorse?WildHorse->Brain->Awareness:-1.f);
            if (RetrySmokeCount<2)
            {
                ++RetrySmokeCount;
                FTimerHandle RetryHandle;
                GetWorldTimerManager().SetTimer(RetryHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]()
                { CastChecked<ASteppePlayerController>(NewPlayer)->SteppeRestartTrial(); }),3.f,false);
            }
        }
        FTimerHandle StartHandle,PresentationCameraHandle,PresentationShotHandle,GrasslandCameraHandle,GrasslandWaterHandle,GrasslandLogHandle,TraversalProbeHandle,TraversalLogHandle,HorseModelSetupHandle,FocusHandle,LassoSetupHandle,LassoAimHandle,LassoThrowHandle,LassoSwingShotHandle,LassoHitShotHandle,BalanceLoadHandle,BalanceShotHandle,BalanceLogHandle,BalanceReleaseHandle,BalanceRecoveryLogHandle,FeedbackReleaseHandle,FeedbackDriveHandle,FeedbackShotHandle,FeedbackHardSurfaceHandle,FeedbackHardShotHandle,BraceHandle,CaptureHandle,DismountHandle,ApproachHandle,ContactHandle,LeadHandle,LeadFinishHandle,LeadShotHandle,CardShotHandle,NameHandle,ArchetypeSetupHandle,ArchetypeShotHandle,GuidanceShotHandle,PostCaptureShotHandle,ShotHandle,ExitHandle;
        if (bGrasslandTraversalSmoke && PlaygroundHorse)
        {
            PlaygroundHorse->SetActorRotation(FRotator(0,90,0));
            NewPlayer->SetControlRotation(FRotator(-8,90,0));
            const TSharedRef<float> Northernmost=MakeShared<float>(PlaygroundHorse->GetActorLocation().Y);
            const TSharedRef<bool> WaterObserved=MakeShared<bool>(false);
            GetWorldTimerManager().SetTimer(StartHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
            { FRidingIntent Intent; Intent.Forward=1.f; Intent.bSprint=true; Rider->Riding->SetIntent(Intent); }),.5f,false);
            GetWorldTimerManager().SetTimer(TraversalProbeHandle,FTimerDelegate::CreateWeakLambda(this,[this,Northernmost,WaterObserved]()
            {
                if (!PlaygroundHorse) { return; }
                *Northernmost=FMath::Max(*Northernmost,PlaygroundHorse->GetActorLocation().Y);
                if (const auto* Movement=Cast<UHorseMovementComponent>(PlaygroundHorse->GetCharacterMovement()))
                { *WaterObserved|=Movement->SurfaceMovementMultiplier<.9f; }
            }),.25f,true);
            GetWorldTimerManager().SetTimer(TraversalLogHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,Northernmost,WaterObserved]()
            {
                const auto* Movement=PlaygroundHorse?Cast<UHorseMovementComponent>(PlaygroundHorse->GetCharacterMovement()):nullptr;
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_TRAVERSAL: Distance=%.0f ReachedHerd=%d WaterObserved=%d Mounted=%d"),
                    *Northernmost+31000.f,*Northernmost>10000.f,*WaterObserved,Rider->Riding->IsMounted());
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_TRAVERSAL_STATE: Location=%s Yaw=%.0f Speed=%.0f Desired=%.0f Brake=%.2f Avoid=%d Obstacle=%.0f Stamina=%.0f Slope=%.1f Mode=%d"),
                    PlaygroundHorse?*PlaygroundHorse->GetActorLocation().ToCompactString():TEXT("Missing"),PlaygroundHorse?PlaygroundHorse->GetActorRotation().Yaw:0.f,
                    Movement?Movement->CurrentSpeed:0.f,Movement?Movement->DesiredSpeed:0.f,Movement?Movement->HorseIntent.BrakeStrength:0.f,
                    Movement && Movement->bRiderAvoidingObstacle,Movement?Movement->RiderObstacleDistance:0.f,
                    PlaygroundHorse && PlaygroundHorse->Attributes?PlaygroundHorse->Attributes->CurrentStamina:0.f,
                    Movement?Movement->GroundSlope:0.f,Movement?static_cast<int32>(Movement->MovementMode):0);
            }),58.f,false);
        }
        if (bGrasslandSmoke)
        {
            GetWorldTimerManager().SetTimer(GrasslandCameraHandle,FTimerDelegate::CreateWeakLambda(this,[this,NewPlayer]()
            {
                const FVector CameraLocation(0,-43000,18000);
                const FVector LookAt(0,-1000,0);
                if (auto* Camera=GetWorld()->SpawnActor<ACameraActor>(CameraLocation,(LookAt-CameraLocation).Rotation())) { NewPlayer->SetViewTarget(Camera); }
            }),.2f,false);
            GetWorldTimerManager().SetTimer(GrasslandWaterHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
            {
                auto* Horse=Rider->Riding->GetHorse();
                ASteppeTerrainZone* Water=nullptr;
                for (TActorIterator<ASteppeTerrainZone> It(GetWorld()); It; ++It) { if (It->ZoneType==ESteppeTerrainZoneType::ShallowWater) { Water=*It; break; } }
                if (!Horse || !Water) { return; }
                Horse->SetActorLocation(Water->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);
                Water->Enter(nullptr,Horse,nullptr,0,false,FHitResult());
                const float Enter=CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->SurfaceMovementMultiplier;
                Water->Leave(nullptr,Horse,nullptr,0);
                const float Leave=CastChecked<UHorseMovementComponent>(Horse->GetCharacterMovement())->SurfaceMovementMultiplier;
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_WATER: Enter=%.2f Leave=%.2f"),Enter,Leave);
            }),.7f,false);
            GetWorldTimerManager().SetTimer(GrasslandLogHandle,FTimerDelegate::CreateWeakLambda(this,[this]()
            {
                int32 Landscapes=0,RouteActors=0,Trees=0,Rocks=0,GrassInstances=0;
                for (TActorIterator<AActor> It(GetWorld()); It; ++It)
                {
                    Landscapes+=It->IsA<ALandscape>();
                    const FString Label=It->GetActorNameOrLabel();
                    RouteActors+=Label.StartsWith(TEXT("P16.6_"));
                    Trees+=Label.StartsWith(TEXT("P16.6_TreeTrunk_"));
                    Rocks+=Label.StartsWith(TEXT("P16.6_Rock_"));
                    if (const auto* Field=Cast<ASteppeGrassField>(*It)) { GrassInstances+=Field->Grass->GetInstanceCount(); }
                }
                float Spread=0.f;
                if (HerdManager && HerdManager->Members.Num()>1)
                {
                    for (int32 A=0;A<HerdManager->Members.Num();++A) for (int32 B=A+1;B<HerdManager->Members.Num();++B)
                    { Spread=FMath::Max(Spread,FVector::Dist2D(HerdManager->Members[A]->GetActorLocation(),HerdManager->Members[B]->GetActorLocation())); }
                }
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_GRASSLAND: Landscape=%d RouteActors=%d Trees=%d Rocks=%d Grass=%d Herd=%d HabitatSpread=%.0f"),
                    Landscapes,RouteActors,Trees,Rocks,GrassInstances,HerdManager?HerdManager->Members.Num():0,Spread);
                auto GroundAt=[this](float X,float Y)
                {
                    FHitResult Hit;
                    FCollisionQueryParams Params(SCENE_QUERY_STAT(SteppeGrasslandGround),false);
                    Params.bReturnPhysicalMaterial=true;
                    GetWorld()->LineTraceSingleByChannel(Hit,FVector(X,Y,3000),FVector(X,Y,-3000),ECC_Visibility,Params);
                    return Hit;
                };
                const FHitResult CampGround=GroundAt(0,-31000);
                const FHitResult RidgeGround=GroundAt(0,28000);
                const FHitResult RiverGround=GroundAt(0,-17500);
                const FHitResult HardGround=GroundAt(17000,-1500);
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_TERRAIN: CampZ=%.0f RidgeZ=%.0f RiverZ=%.0f HardSurface=%d"),
                    CampGround.ImpactPoint.Z,RidgeGround.ImpactPoint.Z,RiverGround.ImpactPoint.Z,
                    HardGround.PhysMaterial.IsValid()?static_cast<int32>(UPhysicalMaterial::DetermineSurfaceType(HardGround.PhysMaterial.Get())):0);
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP166Grassland.png"),true,false);
            }),2.2f,false);
        }
        if (bHorseModelSmoke && WildHorses.Num()>=3)
        {
            GetWorldTimerManager().SetTimer(HorseModelSetupHandle,FTimerDelegate::CreateWeakLambda(this,[this,NewPlayer]()
            {
                const FVector Positions[]={FVector(900,0,100),FVector(1250,0,100),FVector(1600,0,100)};
                for (int32 Index=0; Index<WildHorses.Num(); ++Index)
                {
                    auto* Horse=WildHorses[Index].Get();
                    if (!Horse) { continue; }
                    if (Index<3)
                    {
                        Horse->SetActorLocation(Positions[Index],false,nullptr,ETeleportType::TeleportPhysics);
                        Horse->SetActorRotation(FRotator::ZeroRotator);
                    }
                    else { Horse->SetActorLocation(FVector(8000+Index*300,5000,100),false,nullptr,ETeleportType::TeleportPhysics); }
                    Horse->Brain->SetComponentTickEnabled(false);
                    Horse->GetCharacterMovement()->StopMovementImmediately();
                    Horse->GetCharacterMovement()->SetComponentTickEnabled(false);
                }
                if (PlaygroundHorse) { PlaygroundHorse->SetActorHiddenInGame(true); }
                if (NewPlayer->GetPawn()) { NewPlayer->GetPawn()->SetActorHiddenInGame(true); }
                const FVector CameraLocation(1250,-760,180);
                const FVector LookAt(1250,0,90);
                if (auto* Camera=GetWorld()->SpawnActor<ACameraActor>(CameraLocation,(LookAt-CameraLocation).Rotation())) { NewPlayer->SetViewTarget(Camera); }
                if (auto* DebugVar=IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse"))) { DebugVar->Set(0,ECVF_SetByCode); }
            }),.25f,false);
        }
        if (bArchetypeSmoke && HerdManager)
        {
            GetWorldTimerManager().SetTimer(ArchetypeSetupHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,NewPlayer]()
            {
                const FVector Center=(PlaygroundHorse?PlaygroundHorse->GetActorLocation():Rider->GetActorLocation())+FVector(1400,0,0);
                const FVector Offsets[]={FVector(0,-360,0),FVector(0,0,0),FVector(0,360,0)};
                for (int32 Index=0; Index<FMath::Min(3,WildHorses.Num()); ++Index)
                {
                    WildHorses[Index]->SetActorLocation(Center+Offsets[Index],false,nullptr,ETeleportType::TeleportPhysics);
                    WildHorses[Index]->SetActorRotation(FRotator(0,180,0));
                    WildHorses[Index]->Brain->SetComponentTickEnabled(false);
                    WildHorses[Index]->GetCharacterMovement()->StopMovementImmediately();
                }
                NewPlayer->SetControlRotation(FRotator(-8,0,0));
                if (auto* DebugVar=IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse"))) { DebugVar->Set(1,ECVF_SetByCode); }
            }),.25f,false);
            GetWorldTimerManager().SetTimer(ArchetypeShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP11Archetypes.png"),true,false); }),1.2f,false);
        }
        if (bIsolationSmoke)
        {
            GetWorldTimerManager().SetTimer(FocusHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]()
            { CastChecked<ASteppePlayerController>(NewPlayer)->SteppeFocusTarget(); }),1.2f,false);
        }
        if (bLassoSmoke && HerdManager && !WildHorses.IsEmpty())
        {
            GetWorldTimerManager().SetTimer(LassoSetupHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,NewPlayer,bTimedLassoSmoke,bBalanceSmoke,bFullLoopSmoke]()
            {
                auto* Target=WildHorses.IsEmpty()?nullptr:WildHorses[0].Get();
                if (!Target) { return; }
                const auto* Mount=Rider->Riding->GetHorse();
                const FVector Direction=bBalanceSmoke && Mount?Mount->GetActorRightVector():FRotator(0,NewPlayer->GetControlRotation().Yaw,0).Vector();
                Target->SetActorLocation(Rider->GetActorLocation()+Direction*500.f+FVector(0,0,100.f),false,nullptr,ETeleportType::TeleportPhysics);
                Target->Brain->SetComponentTickEnabled(false);
                Target->GetCharacterMovement()->StopMovementImmediately();
                CastChecked<UHorseMovementComponent>(Target->GetCharacterMovement())->ClearIntent();
                if (bTimedLassoSmoke)
                {
                    Target->GetCharacterMovement()->SetComponentTickEnabled(false);
                }
                HerdManager->IsolationHoldSeconds=.1f;
                HerdManager->SetFocusedHorse(Target);
            }),.7f,false);
            if (bTimedLassoSmoke)
            {
                GetWorldTimerManager().SetTimer(LassoAimHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                { Rider->Lasso->BeginAim(); }),1.f,false);
                if (bLassoSkillSmoke)
                {
                    GetWorldTimerManager().SetTimer(LassoSwingShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
                    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP12Swing.png"),true,false); }),1.45f,false);
                }
                GetWorldTimerManager().SetTimer(LassoThrowHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
                {
                    auto* Target=HerdManager?HerdManager->FocusedHorse.Get():nullptr;
                if (!Target) { return; }
                const FVector Origin=Rider->GetActorLocation()+FVector(0,0,100);
                Rider->Lasso->ThrowFrom(Origin,(Target->GetActorLocation()+FVector(0,0,70)-Origin).GetSafeNormal());
                }),1.6f,false);
                if (bLassoSkillSmoke)
                {
                    GetWorldTimerManager().SetTimer(LassoHitShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
                    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP12LassoSkill.png"),true,false); }),2.2f,false);
                }
            }
            else
            {
                GetWorldTimerManager().SetTimer(LassoThrowHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
                {
                    auto* Target=HerdManager?HerdManager->FocusedHorse.Get():nullptr;
                    if (!Target || !Rider->Lasso->BeginAim()) { return; }
                    const FVector Origin=Rider->GetActorLocation()+FVector(0,0,100);
                    Rider->Lasso->ThrowFrom(Origin,(Target->GetActorLocation()+FVector(0,0,70)-Origin).GetSafeNormal());
                }),1.3f,false);
            }
            if (bRopeFightSmoke)
            {
                GetWorldTimerManager().SetTimer(BraceHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                { Rider->Lasso->SetBracing(true); }),1.55f,false);
            }
            if (bCaptureSmoke)
            {
                GetWorldTimerManager().SetTimer(CaptureHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                { Rider->Lasso->Capture(); }),5.2f,false);
            }
            if (bBalanceSmoke)
            {
                GetWorldTimerManager().SetTimer(BalanceLoadHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
                {
                    auto* Mount=Rider->Riding->GetHorse();
                    auto* Target=HerdManager?HerdManager->FocusedHorse.Get():nullptr;
                    if (!Mount || !Target || Rider->Lasso->State!=ELassoState::Attached) { return; }
                    Rider->Balance->FallThreshold=.18f;
                    Rider->Balance->WarningThreshold=.08f;
                    Rider->Balance->BuildThreshold=0.f;
                    Mount->GetCharacterMovement()->StopMovementImmediately();
                    Mount->GetCharacterMovement()->SetComponentTickEnabled(false);
                    Mount->GetCharacterMovement()->Velocity=Mount->GetActorForwardVector()*1200.f;
                    const FVector Pull=(Target->GetActorLocation()-Mount->GetActorLocation()).GetSafeNormal2D();
                    Target->SetActorLocation(Target->GetActorLocation()+Pull*300.f,false,nullptr,ETeleportType::TeleportPhysics);
                }),2.f,false);
                GetWorldTimerManager().SetTimer(BalanceShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
                { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP12Dragged.png"),true,false); }),2.7f,false);
                GetWorldTimerManager().SetTimer(BalanceLogHandle,FTimerDelegate::CreateWeakLambda(this,[Rider]()
                {
                    UE_LOG(LogSteppe,Display,TEXT("STEPPE_P12_BALANCE_SMOKE: State=%s Mounted=%d Lasso=%s Balance=%.2f Side=%.2f"),
                        *UEnum::GetValueAsString(Rider->Balance->State),Rider->Riding->IsMounted(),*UEnum::GetValueAsString(Rider->Lasso->State),
                        Rider->Balance->Balance/Rider->Balance->FallThreshold,Rider->Balance->LateralPull);
                }),2.75f,false);
                GetWorldTimerManager().SetTimer(BalanceReleaseHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                { Rider->Lasso->Release(); }),3.f,false);
                GetWorldTimerManager().SetTimer(BalanceRecoveryLogHandle,FTimerDelegate::CreateWeakLambda(this,[Rider]()
                {
                    UE_LOG(LogSteppe,Display,TEXT("STEPPE_P12_BALANCE_RELEASE: State=%s Lasso=%s"),
                        *UEnum::GetValueAsString(Rider->Balance->State),*UEnum::GetValueAsString(Rider->Lasso->State));
                }),3.1f,false);
            }
            if (bFeedbackSmoke)
            {
                GetWorldTimerManager().SetTimer(FeedbackReleaseHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                { Rider->Lasso->Release(); }),5.2f,false);
                GetWorldTimerManager().SetTimer(FeedbackDriveHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                {
                    FRidingIntent Intent; Intent.Forward=1.f; Intent.bSprint=true; Rider->Riding->SetIntent(Intent);
                }),2.6f,false);
                GetWorldTimerManager().SetTimer(FeedbackShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
                { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP13Feedback.png"),true,false); }),4.7f,false);
                GetWorldTimerManager().SetTimer(FeedbackHardSurfaceHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
                {
                    FRidingIntent Intent; Rider->Riding->SetIntent(Intent);
                    if (auto* Horse=Rider->Riding->GetHorse())
                    {
                        Horse->GetCharacterMovement()->StopMovementImmediately();
                        Horse->SetActorLocation(FVector(1200,-1400,110),false,nullptr,ETeleportType::TeleportPhysics);
                    }
                }),5.6f,false);
                GetWorldTimerManager().SetTimer(FeedbackHardShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
                { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP13HardSurface.png"),true,false); }),6.3f,false);
            }
        }
        if (bPostCaptureSequence)
        {
            GetWorldTimerManager().SetTimer(DismountHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
            { Rider->Riding->Dismount(); }),5.35f,false);
            GetWorldTimerManager().SetTimer(ApproachHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
            {
                auto* Target=HerdManager && !HerdManager->CapturedHorses.IsEmpty()?HerdManager->CapturedHorses[0].Get():nullptr;
                if (!Target || Rider->Riding->IsMounted()) { return; }
                Rider->SetActorLocation(Target->GetActorLocation()-FVector(200,0,0),false,nullptr,ETeleportType::TeleportPhysics);
                Rider->GetCharacterMovement()->Velocity=FVector::ZeroVector;
            }),5.6f,false);
            GetWorldTimerManager().SetTimer(ContactHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
            { if (HerdManager) { HerdManager->HandleFirstContactInteraction(Rider); } }),8.4f,false);
        }
        if (bFullLoopSequence)
        {
            GetWorldTimerManager().SetTimer(LeadHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,NewPlayer]()
            {
                auto* Target=HerdManager && !HerdManager->CapturedHorses.IsEmpty()?HerdManager->CapturedHorses[0].Get():nullptr;
                if (!Target || !DeliveryZone || !HerdManager) { return; }
                Target->GetCharacterMovement()->SetComponentTickEnabled(true);
                Target->Brain->SetComponentTickEnabled(true);
                HerdManager->HandleFirstContactInteraction(Rider);
                const FVector Direction=FVector(1,0,0);
                FVector DeliveryLocation=Target->GetActorLocation()+Direction*700.f;
                DeliveryLocation.Z=Rider->GetActorLocation().Z;
                DeliveryZone->SetActorLocation(DeliveryLocation);
                Rider->SetActorLocation(DeliveryZone->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);
                Rider->SetActorRotation(Direction.Rotation());
                Rider->GetCharacterMovement()->Velocity=FVector::ZeroVector;
                NewPlayer->SetControlRotation(FRotator(-10,0,0));
            }),8.65f,false);
            GetWorldTimerManager().SetTimer(LeadShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP10Lead.png"),true,false); }),9.4f,false);
            GetWorldTimerManager().SetTimer(LeadFinishHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
            {
                auto* Target=HerdManager && !HerdManager->CapturedHorses.IsEmpty()?HerdManager->CapturedHorses[0].Get():nullptr;
                if (!Target || !DeliveryZone || !Target->Trust || FVector::Dist2D(Target->Trust->LeadStartLocation,Target->GetActorLocation())<300.f) { return; }
                const FVector Center=Target->GetActorLocation()+FVector(250,0,0);
                DeliveryZone->SetActorLocation(Center);
                Rider->SetActorLocation(Center,false,nullptr,ETeleportType::TeleportPhysics);
            }),14.2f,false);
            GetWorldTimerManager().SetTimer(CardShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP10Card.png"),true,false); }),14.6f,false);
            GetWorldTimerManager().SetTimer(NameHandle,FTimerDelegate::CreateWeakLambda(this,[this,NewPlayer]()
            {
                auto* Target=HerdManager && !HerdManager->DeliveredHorses.IsEmpty()?HerdManager->DeliveredHorses[0].Get():nullptr;
                if (!Target || !HerdManager->ConfirmDeliveredHorseName(Target,TEXT("Saran"))) { return; }
                auto* PC=Cast<ASteppePlayerController>(NewPlayer);
                if (PC && PC->NamingWidget)
                {
                    PC->NamingWidget->Configure(Target->Trust);
                    FTimerHandle CloseCardHandle;
                    GetWorldTimerManager().SetTimer(CloseCardHandle,FTimerDelegate::CreateWeakLambda(PC,[PC]()
                    { PC->CloseHorseNaming(); }),.9f,false);
                }
            }),17.2f,false);
        }
        if (bPresentationSmoke)
        {
            const TSharedRef<FQuat> FirstLegPose=MakeShared<FQuat>(FQuat::Identity);
            FTimerHandle FirstLegHandle,SecondLegHandle;
            auto ReadFrontLeg=[this]() -> FQuat
            {
                if (!PlaygroundHorse || !PlaygroundHorse->GetMesh()) { return FQuat::Identity; }
                const auto* Mesh=PlaygroundHorse->GetMesh();
                const int32 Bone=Mesh->GetBoneIndex(TEXT("frontupperleg_l"));
                return Bone==INDEX_NONE?FQuat::Identity:Mesh->GetComponentQuat().Inverse()*Mesh->GetBoneTransform(Bone).GetRotation();
            };
            GetWorldTimerManager().SetTimer(FirstLegHandle,FTimerDelegate::CreateWeakLambda(this,[FirstLegPose,ReadFrontLeg]()
            { *FirstLegPose=ReadFrontLeg(); }),4.2f,false);
            GetWorldTimerManager().SetTimer(SecondLegHandle,FTimerDelegate::CreateWeakLambda(this,[this,FirstLegPose,ReadFrontLeg]()
            {
                const auto* Mesh=PlaygroundHorse?PlaygroundHorse->GetMesh():nullptr;
                const auto* Instance=Mesh?Cast<UHorseAnimInstance>(Mesh->GetAnimInstance()):nullptr;
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_HORSE_ANIM: BoneDelta=%.3f Speed=%.1f Clip=%s"),
                    FMath::RadiansToDegrees(FirstLegPose->AngularDistance(ReadFrontLeg())),
                    PlaygroundHorse?PlaygroundHorse->AnimationData.Speed:0.f,
                    Instance && Instance->GetActiveSequence()?*Instance->GetActiveSequence()->GetName():TEXT("None"));
            }),4.45f,false);
            GetWorldTimerManager().SetTimer(StartHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
            {
                FRidingIntent Intent; Intent.Forward=1.f; Intent.Turn=.72f; Intent.bSprint=true; Rider->Riding->SetIntent(Intent);
            }),1.f,false);
            GetWorldTimerManager().SetTimer(PresentationCameraHandle,FTimerDelegate::CreateWeakLambda(this,[this,NewPlayer]()
            {
                const float SideYaw=PlaygroundHorse?PlaygroundHorse->GetActorRotation().Yaw+90.f:90.f;
                NewPlayer->SetControlRotation(FRotator(-8.f,SideYaw,0));
            }),4.f,false);
            GetWorldTimerManager().SetTimer(PresentationShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP13Presentation.png"),true,false); }),4.5f,false);
        }
        else if (!bHerdIdleSmoke && !bLassoSmoke && !bGrasslandSmoke && !bGrasslandTraversalSmoke)
        {
            GetWorldTimerManager().SetTimer(StartHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
            {
                FRidingIntent Intent; Intent.Forward=1; Rider->Riding->SetIntent(Intent);
            }),1.f,false);
        }
        if (bVerticalSliceSmoke)
        {
            GetWorldTimerManager().SetTimer(GuidanceShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP8Guidance.png"),true,false);
            }),2.5f,false);
        }
        if (bVerticalFailureSmoke)
        {
            GetWorldTimerManager().SetTimer(GuidanceShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP8Urgency.png"),true,false);
            }),2.4f,false);
        }
        if (bPostCaptureSmoke)
        {
            GetWorldTimerManager().SetTimer(PostCaptureShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP9Approach.png"),true,false); }),7.f,false);
        }
        GetWorldTimerManager().SetTimer(ShotHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,bVerticalSliceSmoke,bVerticalFailureSmoke,bPostCaptureSmoke,bFullLoopSequence,bArchetypeSmoke,bLassoSkillSmoke,bFeedbackSmoke,bPresentationSmoke,bMetricsSmoke,bMetricsFailureSmoke,bHorseModelSmoke,bGrasslandSmoke]()
        {
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_SMOKE: Horse=%s Speed=%.1f Mounted=%d"),*GetNameSafe(PlaygroundHorse),PlaygroundHorse?PlaygroundHorse->GetVelocity().Size2D():0.f,PlaygroundHorse && PlaygroundHorse->MountedRider.IsValid());
            if (WildHorse)
            {
                UE_LOG(LogSteppe, Display, TEXT("STEPPE_P2_SMOKE: State=%s Awareness=%.2f Visible=%d Speed=%.1f Config=%s"),
                    *UEnum::GetValueAsString(WildHorse->Brain->State), WildHorse->Brain->Awareness,
                    WildHorse->Brain->bThreatVisible, WildHorse->GetVelocity().Size2D(), *WildHorse->Brain->GetConfig().GetPathName());
            }
            if (HerdManager)
            {
                int32 Alert=0,Fleeing=0,Yielding=0;
                int32 Moving=0,Blocked=0,Recovering=0;
                TSet<int32> HeadingBuckets;
                for (const TObjectPtr<ASteppeWildHorseCharacter>& MemberPtr : HerdManager->Members)
                {
                    const auto* Member=MemberPtr.Get();
                    if (!Member) { continue; }
                    Alert+=Member->Brain->State==EWildHorseState::Alert;
                    Fleeing+=Member->Brain->State==EWildHorseState::Fleeing;
                    Yielding+=Member->Brain->State==EWildHorseState::Yielding;
                    Moving+=Member->GetVelocity().Size2D()>10.f;
                    Blocked+=Member->Brain->bPathBlocked;
                    Recovering+=Member->Brain->bRecoveringFromBlockage;
                    HeadingBuckets.Add(FMath::RoundToInt(Member->GetActorRotation().Yaw/10.f));
                }
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P3_SMOKE: Members=%d Alert=%d Yielding=%d Fleeing=%d Moving=%d Headings=%d Blocked=%d Recovering=%d Sources=%d MinSpacing=%.1f Spread=%.1f Coherence=%.2f"),
                    HerdManager->Members.Num(),Alert,Yielding,Fleeing,Moving,HeadingBuckets.Num(),Blocked,Recovering,HerdManager->AlarmSourceCount,HerdManager->MinimumMemberSpacing,
                    HerdManager->Members.Num()>1?FVector::Dist2D(HerdManager->Members[0]->GetActorLocation(),HerdManager->Members.Last()->GetActorLocation()):0.f,
                    HerdManager->MovementCoherence);
                const int32 FocusIndex=WildHorses.IndexOfByKey(HerdManager->FocusedHorse);
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P4_SMOKE: Focus=%s Distance=%.1f Progress=%.2f Isolated=%d"),
                    FocusIndex==INDEX_NONE?TEXT("None"):*FString::Printf(TEXT("H%d"),FocusIndex+1),
                    HerdManager->IsolationDistance,HerdManager->IsolationProgress,HerdManager->bTargetIsolated);
            }
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_P5_SMOKE: State=%s Target=%s TargetLassoed=%d Feedback=%s"),
                *UEnum::GetValueAsString(Rider->Lasso->State),*GetNameSafe(Rider->Lasso->Target.Get()),
                Rider->Lasso->Target.IsValid() && Rider->Lasso->Target->Brain->bLassoed,*Rider->Lasso->Feedback);
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_P6_SMOKE: State=%s Tension=%.2f Control=%.2f Bracing=%d"),
                *UEnum::GetValueAsString(Rider->Lasso->State),Rider->Lasso->Tension,Rider->Lasso->ControlProgress,Rider->Lasso->bBracing);
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_P7_SMOKE: State=%s Captured=%d Active=%d TargetState=%s"),
                *UEnum::GetValueAsString(Rider->Lasso->State),HerdManager?HerdManager->CapturedCount:0,
                HerdManager?HerdManager->Members.Num():0,*UEnum::GetValueAsString(Rider->Lasso->Target.IsValid()?Rider->Lasso->Target->Brain->State:EWildHorseState::Roaming));
            if (bVerticalSliceSmoke || bVerticalFailureSmoke)
            {
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P8_SMOKE: State=%s Captured=%d Required=%d Remaining=%.1f Score=%d"),
                    *UEnum::GetValueAsString(Trial.State),Trial.Captured,Trial.RequiredCaptures,Trial.RemainingSeconds,Trial.Score);
            }
            if (bPostCaptureSmoke)
            {
                const auto* Target=HerdManager && !HerdManager->CapturedHorses.IsEmpty()?HerdManager->CapturedHorses[0].Get():nullptr;
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P9_SMOKE: PostState=%s FirstContact=%d FirstContacts=%d Mounted=%d Calm=%.2f Trial=%s"),
                    Target && Target->Trust?*UEnum::GetValueAsString(Target->Trust->State):TEXT("Missing"),
                    Target && Target->Trust && Target->Trust->bFirstContact,HerdManager?HerdManager->FirstContactCount:0,
                    Rider->Riding->IsMounted(),Target && Target->Trust?Target->Trust->CalmProgress:0.f,*UEnum::GetValueAsString(Trial.State));
            }
            if (bFullLoopSequence)
            {
                const auto* Target=HerdManager && !HerdManager->CapturedHorses.IsEmpty()?HerdManager->CapturedHorses[0].Get():nullptr;
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P10_ZONE: Rider=%d Horse=%d Delta=%s RiderZ=%.0f HorseZ=%.0f ZoneZ=%.0f"),
                    DeliveryZone && DeliveryZone->ContainsActor(Rider),DeliveryZone && DeliveryZone->ContainsActor(Target),
                    Target && DeliveryZone?*(Target->GetActorLocation()-DeliveryZone->GetActorLocation()).ToCompactString():TEXT("Missing"),
                    Rider->GetActorLocation().Z,Target?Target->GetActorLocation().Z:0.f,DeliveryZone?DeliveryZone->GetActorLocation().Z:0.f);
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P10_SMOKE: PostState=%s Leading=%d Delivered=%d Named=%d HorseName=%s Travel=%.1f Trial=%s"),
                    Target && Target->Trust?*UEnum::GetValueAsString(Target->Trust->State):TEXT("Missing"),
                    Target && Target->Trust && Target->Trust->bLeading,HerdManager?HerdManager->DeliveredCount:0,HerdManager?HerdManager->NamedCount:0,
                    Target && Target->Trust?*Target->Trust->HorseName:TEXT(""),Target && Target->Trust?FVector::Dist2D(Target->Trust->LeadStartLocation,Target->GetActorLocation()):0.f,
                    *UEnum::GetValueAsString(Trial.State));
            }
            if (bArchetypeSmoke && WildHorses.Num()>=3)
            {
                FString Summary=TEXT("STEPPE_P11_SMOKE:");
                for (int32 Index=0; Index<3; ++Index)
                {
                    const auto* Horse=WildHorses[Index].Get();
                    Summary+=FString::Printf(TEXT(" H%d=%s Speed=%.0f Strength=%.2f Fear=%.2f/%.2f Safe=%.0f Calm=%.2f"),Index+1,
                        *Horse->ArchetypeLabel,Horse->Attributes->MaxSpeed,Horse->Attributes->Strength,Horse->Brain->AwarenessRiseScale,
                        Horse->Brain->AwarenessDecayScale,Horse->Trust->SafeApproachSpeed,Horse->Trust->CalmHoldSeconds);
                }
                UE_LOG(LogSteppe,Display,TEXT("%s"),*Summary);
            }
            if (bLassoSkillSmoke)
            {
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P12_SMOKE: Stability=%.2f Zone=%s Radius=%.1f Range=%.1f"),
                    Rider->Lasso->LastThrowStability,*UEnum::GetValueAsString(Rider->Lasso->HitZone),
                    Rider->Lasso->EffectiveCaptureRadius,Rider->Lasso->EffectiveMaximumRange);
            }
            if (bFeedbackSmoke && Rider->Feedback)
            {
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P13_SMOKE: Hoofbeats=%d LassoEvents=%d RiskEvents=%d Wind=%.2f Breath=%.2f Rope=%.2f Last=%s Surface=%s"),
                    Rider->Feedback->HoofbeatCount,Rider->Feedback->LassoEventCount,Rider->Feedback->RiskEventCount,
                    Rider->Feedback->WindIntensity,Rider->Feedback->BreathIntensity,Rider->Feedback->RopeStress,
                    *UEnum::GetValueAsString(Rider->Feedback->LastEvent),*UEnum::GetValueAsString(Rider->Feedback->GroundSurface));
            }
            if (bPresentationSmoke && PlaygroundHorse)
            {
                const auto& Pose=PlaygroundHorse->AnimationData;
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P13_PRESENTATION: Gait=%s Phase=%.2f Stride=%.2f Bob=%.1f Roll=%.1f RiderRoll=%.1f Mounted=%d"),
                    *UEnum::GetValueAsString(Pose.Gait),Pose.GaitPhase,Pose.StrideBlend,Pose.BodyBob,Pose.BodyRoll,
                    Rider->PresentationData.BodyRoll,Rider->PresentationData.bMounted);
            }
            if (bMetricsSmoke && PlaytestMetrics)
            {
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P14_SUMMARY: %s"),*PlaytestMetrics->GetCompactSummary());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(bMetricsFailureSmoke?TEXT("Screenshots/SteppeP14Failure.png"):TEXT("Screenshots/SteppeP14Success.png")),true,false);
            }
            if (bHorseModelSmoke && WildHorses.Num()>=3)
            {
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P14_HORSE_MODEL: Parts=%d Legs=%d H1=%s H2=%s H3=%s"),
                    WildHorses[0]->GetPlaceholderPartCount(),WildHorses[0]->GetPlaceholderLegCount(),*WildHorses[0]->ArchetypeLabel,*WildHorses[1]->ArchetypeLabel,*WildHorses[2]->ArchetypeLabel);
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP14HorseModel.png"),true,false);
            }
            if (!bMetricsSmoke && !bHorseModelSmoke && !bGrasslandSmoke) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeSmoke.png"),true,false); }
        }),bGrasslandTraversalSmoke?58.5f:(bGrasslandSmoke?2.5f:(bHorseModelSmoke?1.5f:(bArchetypeSmoke?1.5f:(bHerdIdleSmoke?3.5f:(bVerticalFailureSmoke?4.f:(bFullLoopSequence?18.5f:(bPostCaptureSequence?10.f:7.f))))))),false);
        GetWorldTimerManager().SetTimer(ExitHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]() { NewPlayer->ConsoleCommand(TEXT("quit")); }),bGrasslandTraversalSmoke?60.f:(bGrasslandSmoke?4.f:(bHorseModelSmoke?3.f:(bArchetypeSmoke?3.f:(bHerdIdleSmoke?5.5f:(bVerticalFailureSmoke?6.f:(bFullLoopSequence?20.f:(bPostCaptureSequence?12.f:9.f))))))),false);
    }
}
