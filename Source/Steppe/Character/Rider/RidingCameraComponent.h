#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RidingCameraComponent.generated.h"
UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API URidingCameraComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URidingCameraComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
};
