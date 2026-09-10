#include "Debug/SteppeDebugSubsystem.h"
#include "HAL/IConsoleManager.h"
static TAutoConsoleVariable<int32> CVarHorseDebug(TEXT("steppe.Debug.Horse"), 0, TEXT("Show horse telemetry: 0/1"));
static TAutoConsoleVariable<int32> CVarMovementDebug(TEXT("steppe.Debug.Movement"), 0, TEXT("Show horse movement vectors: 0/1"));
bool USteppeDebugSubsystem::IsHorseDebugEnabled() const { return CVarHorseDebug.GetValueOnGameThread() != 0; }
bool USteppeDebugSubsystem::IsMovementDebugEnabled() const { return CVarMovementDebug.GetValueOnGameThread() != 0; }
