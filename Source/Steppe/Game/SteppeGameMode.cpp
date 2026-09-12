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
ASteppeGameMode::ASteppeGameMode()
{
    PlayerControllerClass = ASteppePlayerController::StaticClass();
    DefaultPawnClass=ASteppeRiderCharacter::StaticClass();
    HUDClass=ASteppeHUD::StaticClass();
    HorseClass=ASteppeHorseCharacter::StaticClass();
    WildHorseClass=ASteppeWildHorseCharacter::StaticClass();
}
void ASteppeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
    auto* Rider=Cast<ASteppeRiderCharacter>(NewPlayer->GetPawn()); if (!Rider) { return; }
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
            WildHorses=HerdManager->Members;
            WildHorse=WildHorses.IsEmpty()?nullptr:WildHorses[0];
        }
        if (!WildHorse) { UE_LOG(LogSteppe, Error, TEXT("Wild herd spawn failed; check WildHorseClass and spawn clearance.")); }
    }
    if (bDebugEnabled) { IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse"))->Set(1,ECVF_SetByCode); }
    if (FParse::Param(FCommandLine::Get(),TEXT("SteppeSmoke")))
    {
        // Explicit development smoke mode; normal play never injects input or exits.
        FApp::SetUseFixedTimeStep(true);
        FApp::SetFixedDeltaTime(1.0/60.0);
        NewPlayer->SetControlRotation(FRotator(-12,25,0));
        const bool bHerdIdleSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeHerdIdleSmoke"));
        const bool bIsolationSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeIsolationSmoke"));
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
        FTimerHandle StartHandle,FocusHandle,ShotHandle,ExitHandle;
        if (bIsolationSmoke)
        {
            GetWorldTimerManager().SetTimer(FocusHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]()
            { CastChecked<ASteppePlayerController>(NewPlayer)->SteppeFocusTarget(); }),1.2f,false);
        }
        if (!bHerdIdleSmoke)
        {
            GetWorldTimerManager().SetTimer(StartHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
            {
                FRidingIntent Intent; Intent.Forward=1; Rider->Riding->SetIntent(Intent);
            }),1.f,false);
        }
        GetWorldTimerManager().SetTimer(ShotHandle,FTimerDelegate::CreateWeakLambda(this,[this]()
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
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeSmoke.png"),true,false);
        }),bHerdIdleSmoke?3.5f:7.f,false);
        GetWorldTimerManager().SetTimer(ExitHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]() { NewPlayer->ConsoleCommand(TEXT("quit")); }),bHerdIdleSmoke?5.5f:9.f,false);
    }
}
