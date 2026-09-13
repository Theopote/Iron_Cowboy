#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "HorseNamingWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UHorseTrustComponent;

UCLASS()
class STEPPE_API UHorseNamingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(UHorseTrustComponent* InHorse);
    void FocusNameInput();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void RefreshCard();
    UFUNCTION() void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    UFUNCTION() void HandleConfirmClicked();
    UFUNCTION() void HandleReplayClicked();
    void TryConfirm();

    UPROPERTY() TWeakObjectPtr<UHorseTrustComponent> Horse;
    UPROPERTY() TObjectPtr<UTextBlock> CardText;
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
    UPROPERTY() TObjectPtr<UEditableTextBox> NameInput;
    UPROPERTY() TObjectPtr<UButton> ConfirmButton;
};
