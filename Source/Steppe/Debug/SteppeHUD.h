#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SteppeHUD.generated.h"
UCLASS()
class STEPPE_API ASteppeHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
