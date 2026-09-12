#include "Player/SteppePlayerController.h"
#include "HAL/IConsoleManager.h"
void ASteppePlayerController::SteppeToggleDebug()
{
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("steppe.Debug.Horse")))
    {
        Variable->Set(Variable->GetInt() == 0 ? 1 : 0, ECVF_SetByConsole);
    }
}

void ASteppePlayerController::SteppeRestartTrial()
{
    // Reload the current standalone world so rider, horse, AI and timers reset together.
    RestartLevel();
}
