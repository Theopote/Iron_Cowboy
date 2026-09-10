#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SteppePlayerController.generated.h"
UCLASS()
class STEPPE_API ASteppePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UFUNCTION(Exec) void SteppeToggleDebug();
};
