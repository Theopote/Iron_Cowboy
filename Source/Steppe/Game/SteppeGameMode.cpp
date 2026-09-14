#include "Game/SteppeGameMode.h"
#include "Player/SteppePlayerController.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
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
ASteppeGameMode::ASteppeGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    PlayerControllerClass = ASteppePlayerController::StaticClass();
    DefaultPawnClass=ASteppeRiderCharacter::StaticClass();
    HUDClass=ASteppeHUD::StaticClass();
    HorseClass=ASteppeHorseCharacter::StaticClass();
    WildHorseClass=ASteppeWildHorseCharacter::StaticClass();
    DeliveryZoneClass=ASteppeDeliveryZone::StaticClass();
}
void ASteppeGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bEnableTrial && Trial.State==ESteppeTrialState::Running)
    {
        Trial.Advance(DeltaSeconds,HerdManager?HerdManager->CapturedCount:0,HerdManager?HerdManager->FirstContactCount:0,
            HerdManager?HerdManager->DeliveredCount:0,HerdManager?HerdManager->NamedCount:0);
    }
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
        const bool bTimedLassoSmoke=bLassoSkillSmoke || bBalanceSmoke || bFeedbackSmoke;
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
        FTimerHandle StartHandle,PresentationCameraHandle,PresentationShotHandle,FocusHandle,LassoSetupHandle,LassoAimHandle,LassoThrowHandle,LassoSwingShotHandle,LassoHitShotHandle,BalanceLoadHandle,BalanceShotHandle,BalanceLogHandle,BalanceReleaseHandle,BalanceRecoveryLogHandle,FeedbackReleaseHandle,FeedbackDriveHandle,FeedbackShotHandle,FeedbackHardSurfaceHandle,FeedbackHardShotHandle,BraceHandle,CaptureHandle,DismountHandle,ApproachHandle,ContactHandle,LeadHandle,LeadShotHandle,CardShotHandle,NameHandle,ArchetypeSetupHandle,ArchetypeShotHandle,GuidanceShotHandle,PostCaptureShotHandle,ShotHandle,ExitHandle;
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
            GetWorldTimerManager().SetTimer(LassoSetupHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,NewPlayer,bTimedLassoSmoke,bBalanceSmoke]()
            {
                auto* Target=WildHorses.IsEmpty()?nullptr:WildHorses[0].Get();
                if (!Target) { return; }
                const auto* Mount=Rider->Riding->GetHorse();
                const FVector Direction=bBalanceSmoke && Mount?Mount->GetActorRightVector():FRotator(0,NewPlayer->GetControlRotation().Yaw,0).Vector();
                Target->SetActorLocation(Rider->GetActorLocation()+Direction*1200.f,false,nullptr,ETeleportType::TeleportPhysics);
                if (bTimedLassoSmoke)
                {
                    Target->Brain->SetComponentTickEnabled(false);
                    Target->GetCharacterMovement()->StopMovementImmediately();
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
                HerdManager->HandleFirstContactInteraction(Rider);
                const FVector Direction=FVector(1,0,0);
                DeliveryZone->SetActorLocation(Target->GetActorLocation()+Direction*1000.f);
                Rider->SetActorLocation(DeliveryZone->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);
                Rider->SetActorRotation(Direction.Rotation());
                Rider->GetCharacterMovement()->Velocity=FVector::ZeroVector;
                NewPlayer->SetControlRotation(FRotator(-10,180,0));
            }),8.65f,false);
            GetWorldTimerManager().SetTimer(LeadShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP10Lead.png"),true,false); }),9.4f,false);
            GetWorldTimerManager().SetTimer(CardShotHandle,FTimerDelegate::CreateWeakLambda(this,[]()
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeP10Card.png"),true,false); }),11.5f,false);
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
            }),12.f,false);
        }
        if (bPresentationSmoke)
        {
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
        else if (!bHerdIdleSmoke && !bLassoSmoke)
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
        GetWorldTimerManager().SetTimer(ShotHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,bVerticalSliceSmoke,bVerticalFailureSmoke,bPostCaptureSmoke,bFullLoopSequence,bArchetypeSmoke,bLassoSkillSmoke,bFeedbackSmoke,bPresentationSmoke]()
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
                UE_LOG(LogSteppe,Display,TEXT("STEPPE_P3_SMOKE: Members=%d Alert=%d Yielding=%d Fleeing=%d Moving=%d Headings=%d Blocked=%d Recovering=%d Sources=%d MinSpacing=%.1f Spread=%.1f"),
                    HerdManager->Members.Num(),Alert,Yielding,Fleeing,Moving,HeadingBuckets.Num(),Blocked,Recovering,HerdManager->AlarmSourceCount,HerdManager->MinimumMemberSpacing,
                    HerdManager->Members.Num()>1?FVector::Dist2D(HerdManager->Members[0]->GetActorLocation(),HerdManager->Members.Last()->GetActorLocation()):0.f);
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
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeSmoke.png"),true,false);
        }),bArchetypeSmoke?1.5f:(bHerdIdleSmoke?3.5f:(bVerticalFailureSmoke?4.f:(bFullLoopSequence?13.f:(bPostCaptureSequence?10.f:7.f)))),false);
        GetWorldTimerManager().SetTimer(ExitHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]() { NewPlayer->ConsoleCommand(TEXT("quit")); }),bArchetypeSmoke?3.f:(bHerdIdleSmoke?5.5f:(bVerticalFailureSmoke?6.f:(bFullLoopSequence?15.f:(bPostCaptureSequence?12.f:9.f)))),false);
    }
}
