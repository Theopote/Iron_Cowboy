#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SteppeLandscapeBuilder.generated.h"
UCLASS()
class STEPPE_API USteppeLandscapeBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Steppe|Editor") static bool BuildGrasslandBlockout();
};
