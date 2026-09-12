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
        if (!IsValid(WildHorse))
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
            WildHorse = GetWorld()->SpawnActor<ASteppeWildHorseCharacter>(WildHorseClass, WildHorseSpawnTransform, Params);
        }
        if (WildHorse) { WildHorse->Brain->SetThreatTarget(Rider); }
        else { UE_LOG(LogSteppe, Error, TEXT("Wild horse spawn failed; check WildHorseClass and spawn clearance.")); }
    }
    if (bDebugEnabled) { IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse"))->Set(1,ECVF_SetByCode); }
    if (FParse::Param(FCommandLine::Get(),TEXT("SteppeSmoke")))
    {
        // Explicit development smoke mode; normal play never injects input or exits.
        FApp::SetUseFixedTimeStep(true);
        FApp::SetFixedDeltaTime(1.0/60.0);
        NewPlayer->SetControlRotation(FRotator(-12,25,0));
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
        FTimerHandle StartHandle,ShotHandle,ExitHandle;
        GetWorldTimerManager().SetTimer(StartHandle,FTimerDelegate::CreateWeakLambda(Rider,[Rider]()
        {
            FRidingIntent Intent; Intent.Forward=1; Rider->Riding->SetIntent(Intent);
        }),1.f,false);
        GetWorldTimerManager().SetTimer(ShotHandle,FTimerDelegate::CreateWeakLambda(this,[this]()
        {
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_SMOKE: Horse=%s Speed=%.1f Mounted=%d"),*GetNameSafe(PlaygroundHorse),PlaygroundHorse?PlaygroundHorse->GetVelocity().Size2D():0.f,PlaygroundHorse && PlaygroundHorse->MountedRider.IsValid());
            if (WildHorse)
            {
                UE_LOG(LogSteppe, Display, TEXT("STEPPE_P2_SMOKE: State=%s Awareness=%.2f Visible=%d Speed=%.1f Config=%s"),
                    *UEnum::GetValueAsString(WildHorse->Brain->State), WildHorse->Brain->Awareness,
                    WildHorse->Brain->bThreatVisible, WildHorse->GetVelocity().Size2D(), *WildHorse->Brain->GetConfig().GetPathName());
            }
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/SteppeSmoke.png"),true,false);
        }),7.f,false);
        GetWorldTimerManager().SetTimer(ExitHandle,FTimerDelegate::CreateWeakLambda(NewPlayer,[NewPlayer]() { NewPlayer->ConsoleCommand(TEXT("quit")); }),9.f,false);
    }
}
