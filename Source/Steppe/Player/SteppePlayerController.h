#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SteppePlayerController.generated.h"
class UHorseTrustComponent;
class UHorseNamingWidget;
UCLASS()
class STEPPE_API ASteppePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UFUNCTION(Exec) void SteppeToggleDebug();
    UFUNCTION(Exec) void SteppeFocusTarget();
    UFUNCTION(Exec) void SteppeRestartTrial();
    void ShowHorseNaming(UHorseTrustComponent* Horse);
    UPROPERTY() TObjectPtr<UHorseNamingWidget> NamingWidget;
};
