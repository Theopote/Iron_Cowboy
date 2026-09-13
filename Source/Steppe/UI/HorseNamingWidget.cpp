#include "UI/HorseNamingWidget.h"
#include "AI/SteppeHerdManager.h"
#include "Capture/HorseTrustComponent.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Game/SteppeGameMode.h"
#include "Player/SteppePlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void UHorseNamingWidget::Configure(UHorseTrustComponent* InHorse)
{
    Horse=InHorse;
    RefreshCard();
}

void UHorseNamingWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NamingCanvas"));
        WidgetTree->RootWidget=Canvas;
        auto* Border=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("CardBorder"));
        Border->SetBrushColor(FLinearColor(.025f,.035f,.03f,.96f));
        Border->SetPadding(FMargin(28.f));
        auto* CanvasSlot=Canvas->AddChildToCanvas(Border);
        CanvasSlot->SetAnchors(FAnchors(.5f,.5f));
        CanvasSlot->SetAlignment(FVector2D(.5f,.5f));
        CanvasSlot->SetSize(FVector2D(520.f,490.f));

        auto* Stack=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("CardStack"));
        Border->SetContent(Stack);
        auto AddText=[this,Stack](FName Name, const FString& Value, FLinearColor Color)
        {
            auto* Label=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
            Label->SetText(FText::FromString(Value));
            Label->SetColorAndOpacity(FSlateColor(Color));
            Label->SetAutoWrapText(true);
            Stack->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,5));
            return Label;
        };
        AddText(TEXT("Title"),TEXT("HORSE CARD"),FLinearColor(1.f,.82f,.25f));
        CardText=AddText(TEXT("CardText"),TEXT("Preparing horse record..."),FLinearColor(.88f,.95f,.9f));
        AddText(TEXT("NameLabel"),TEXT("Give this horse a name (1-16 characters)"),FLinearColor(.55f,1.f,1.f));
        NameInput=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("NameInput"));
        NameInput->SetHintText(FText::FromString(TEXT("Enter name")));
        NameInput->OnTextCommitted.AddDynamic(this,&UHorseNamingWidget::HandleNameCommitted);
        Stack->AddChildToVerticalBox(NameInput)->SetPadding(FMargin(0,8));
        ConfirmButton=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("ConfirmButton"));
        auto* ConfirmLabel=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("ConfirmLabel"));
        ConfirmLabel->SetText(FText::FromString(TEXT("CONFIRM NAME")));
        ConfirmLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
        ConfirmButton->AddChild(ConfirmLabel);
        ConfirmButton->OnClicked.AddDynamic(this,&UHorseNamingWidget::HandleConfirmClicked);
        Stack->AddChildToVerticalBox(ConfirmButton)->SetPadding(FMargin(0,8));
        StatusText=AddText(TEXT("Status"),TEXT("Press Enter to confirm | F2 replay"),FLinearColor(.75f,.8f,.75f));
    }
    RefreshCard();
}

void UHorseNamingWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshCard();
}

void UHorseNamingWidget::RefreshCard()
{
    if (!CardText || !Horse.IsValid()) { return; }
    const auto* Wild=Cast<ASteppeWildHorseCharacter>(Horse->GetOwner());
    const auto* A=Wild?Wild->Attributes.Get():nullptr;
    CardText->SetText(FText::FromString(FString::Printf(
        TEXT("%s\n%s | %d years | %s\nTemperament: %s\n\nSpeed      %.0f cm/s\nEndurance  %.0f\nStrength   %.1f\nAgility    %.1f"),
        *Horse->HorseId,*Horse->Sex,Horse->AgeYears,*Horse->Coat,*Horse->Temperament,
        A?A->MaxSpeed:0.f,A?A->MaxStamina:0.f,A?A->Strength:0.f,A?A->Agility:0.f)));
    if (Horse->bNamed && NameInput && StatusText && ConfirmButton)
    {
        NameInput->SetText(FText::FromString(Horse->HorseName));
        NameInput->SetIsReadOnly(true);
        ConfirmButton->SetIsEnabled(false);
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("WELCOME, %s | F2 replay"),*Horse->HorseName.ToUpper())));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(.3f,1.f,.4f)));
    }
}

void UHorseNamingWidget::FocusNameInput()
{
    if (NameInput) { NameInput->SetKeyboardFocus(); }
}

void UHorseNamingWidget::HandleNameCommitted(const FText&, ETextCommit::Type CommitMethod)
{
    if (CommitMethod==ETextCommit::OnEnter) { TryConfirm(); }
}

void UHorseNamingWidget::HandleConfirmClicked() { TryConfirm(); }

void UHorseNamingWidget::TryConfirm()
{
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ASteppeGameMode>():nullptr;
    if (!Mode || !Mode->HerdManager || !Horse.IsValid() || !NameInput) { return; }
    if (!Mode->HerdManager->ConfirmDeliveredHorseName(Horse->GetOwner(),NameInput->GetText().ToString()))
    {
        StatusText->SetText(FText::FromString(Horse->Feedback));
        return;
    }
    RefreshCard();
}

void UHorseNamingWidget::HandleReplayClicked()
{
    if (auto* PC=GetOwningPlayer<ASteppePlayerController>()) { PC->SteppeRestartTrial(); }
}

FReply UHorseNamingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey()==EKeys::F2)
    {
        HandleReplayClicked();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry,InKeyEvent);
}
