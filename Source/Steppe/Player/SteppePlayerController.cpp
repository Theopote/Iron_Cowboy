#include "Player/SteppePlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Game/SteppeGameMode.h"
#include "AI/SteppeHerdManager.h"
#include "Engine/World.h"
void ASteppePlayerController::SteppeToggleDebug()
{
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse")))
    {
        Variable->Set(Variable->GetInt() == 0 ? 1 : 0, ECVF_SetByConsole);
    }
}

void ASteppePlayerController::SteppeFocusTarget()
{
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr;
    if (!Mode || !Mode->HerdManager || !GetPawn()) { return; }
    Mode->HerdManager->SelectFocusHorse(GetPawn()->GetActorLocation(),GetControlRotation().Vector());
}

void ASteppePlayerController::SteppeRestartTrial()
{
    // Reload the current standalone world so rider, horse, AI and timers reset together.
    RestartLevel();
}
