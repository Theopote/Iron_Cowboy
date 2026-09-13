#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/Rider/RidingIntent.h"
#include "RidingComponent.generated.h"
class ASteppeHorseCharacter;
UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API URidingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URidingComponent();
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintCallable) bool TryMount(ASteppeHorseCharacter* Horse);
    UFUNCTION(BlueprintCallable) void Dismount();
    bool ForceDismount(FVector LaunchVelocity);
    UFUNCTION(BlueprintPure) bool IsMounted() const { return MountedHorse.IsValid(); }
    UFUNCTION(BlueprintPure) ASteppeHorseCharacter* GetHorse() const { return MountedHorse.Get(); }
    void SetIntent(const FRidingIntent& NewIntent) { Intent=NewIntent; Intent.Clamp(); }
    UPROPERTY(EditDefaultsOnly, Category="Riding") float MountDistance = 350.f;
    UPROPERTY(EditDefaultsOnly, Category="Riding") float DismountMaxSpeed = 200.f;
    UPROPERTY(EditDefaultsOnly, Category="Riding") float DismountOffset = 180.f;
private:
    UFUNCTION() void OnHorseDestroyed(AActor* Actor);
    UPROPERTY() TWeakObjectPtr<ASteppeHorseCharacter> MountedHorse;
    FRidingIntent Intent;
};
