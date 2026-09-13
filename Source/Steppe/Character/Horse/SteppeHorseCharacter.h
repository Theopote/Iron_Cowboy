#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Character/Horse/HorseMovementTypes.h"
#include "GameplayTagContainer.h"
#include "SteppeHorseCharacter.generated.h"
class UHorseAttributeComponent;
class UHorseLocomotionConfig;
class UStaticMeshComponent;
UCLASS()
class STEPPE_API ASteppeHorseCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ASteppeHorseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Horse") TObjectPtr<UHorseAttributeComponent> Attributes;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse") TObjectPtr<UHorseLocomotionConfig> LocomotionConfig;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Horse|Animation") FHorseAnimationData AnimationData;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Riding") FName RiderSocket = TEXT("RiderSeat");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Riding") FTransform FallbackSeat = FTransform(FVector(0,0,110));
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Riding") TWeakObjectPtr<ACharacter> MountedRider;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Riding") bool bCanBeMounted = true;
    const UHorseLocomotionConfig* GetLocomotionConfig() const;
    UFUNCTION(BlueprintPure, Category="Horse") FGameplayTag GetGaitTag() const;
    UFUNCTION(BlueprintPure, Category="Horse") FGameplayTag GetStateTag() const;
    UStaticMeshComponent* GetPlaceholderMesh() const { return Placeholder; }
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Placeholder;
};
