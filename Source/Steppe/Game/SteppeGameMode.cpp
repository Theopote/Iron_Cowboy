#include "Game/SteppeGameMode.h"
#include "Player/SteppePlayerController.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
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
        FTimerHandle StartHandle,FocusHandle,LassoSetupHandle,LassoThrowHandle,BraceHandle,CaptureHandle,DismountHandle,ApproachHandle,ContactHandle,LeadHandle,LeadShotHandle,CardShotHandle,NameHandle,GuidanceShotHandle,PostCaptureShotHandle,ShotHandle,ExitHandle;
        if (bIsolationSmoke)
        {
            GetWorldTimerManager().SetTimer(FocusHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]()
            { CastChecked<ASteppePlayerController>(NewPlayer)->SteppeFocusTarget(); }),1.2f,false);
        }
        if (bLassoSmoke && HerdManager && !WildHorses.IsEmpty())
        {
            GetWorldTimerManager().SetTimer(LassoSetupHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,NewPlayer]()
            {
                auto* Target=WildHorses.IsEmpty()?nullptr:WildHorses[0].Get();
                if (!Target) { return; }
                const FVector Direction=FRotator(0,NewPlayer->GetControlRotation().Yaw,0).Vector();
                Target->SetActorLocation(Rider->GetActorLocation()+Direction*1200.f,false,nullptr,ETeleportType::TeleportPhysics);
                HerdManager->IsolationHoldSeconds=.1f;
                HerdManager->SetFocusedHorse(Target);
            }),.7f,false);
            GetWorldTimerManager().SetTimer(LassoThrowHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider]()
            {
                auto* Target=HerdManager?HerdManager->FocusedHorse.Get():nullptr;
                if (!Target || !Rider->Lasso->BeginAim()) { return; }
                const FVector Origin=Rider->GetActorLocation()+FVector(0,0,100);
                Rider->Lasso->ThrowFrom(Origin,(Target->GetActorLocation()+FVector(0,0,70)-Origin).GetSafeNormal());
            }),1.3f,false);
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
                if (PC && PC->NamingWidget) { PC->NamingWidget->Configure(Target->Trust); }
            }),12.f,false);
        }
        if (!bHerdIdleSmoke && !bLassoSmoke)
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
        GetWorldTimerManager().SetTimer(ShotHandle,FTimerDelegate::CreateWeakLambda(this,[this,Rider,bVerticalSliceSmoke,bVerticalFailureSmoke,bPostCaptureSmoke,bFullLoopSequence]()
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
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeSmoke.png"),true,false);
        }),bHerdIdleSmoke?3.5f:(bVerticalFailureSmoke?4.f:(bFullLoopSequence?13.f:(bPostCaptureSequence?10.f:7.f))),false);
        GetWorldTimerManager().SetTimer(ExitHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]() { NewPlayer->ConsoleCommand(TEXT("quit")); }),bHerdIdleSmoke?5.5f:(bVerticalFailureSmoke?6.f:(bFullLoopSequence?15.f:(bPostCaptureSequence?12.f:9.f))),false);
    }
}
