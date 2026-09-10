#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SteppeInputConfig.generated.h"
class UInputAction;
class UInputMappingContext;
UCLASS(BlueprintType)
class STEPPE_API USteppeInputConfig : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> Move;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> Look;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> Sprint;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> Brake;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> Interact;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> MountDismount;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputAction> Debug;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputMappingContext> OnFoot;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UInputMappingContext> Riding;
    void CreateRuntimeDefaults();
};
