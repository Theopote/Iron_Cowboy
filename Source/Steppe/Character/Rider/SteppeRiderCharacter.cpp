#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Rider/RidingComponent.h"
#include "Character/Rider/RidingCameraComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Input/SteppeInputConfig.h"
#include "Player/SteppePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Steppe.h"
#include "Core/SteppeGameplayTags.h"
#include "Lasso/LassoComponent.h"
#include "Character/Rider/RiderBalanceComponent.h"
#include "Feedback/SteppeFeedbackComponent.h"
#include "Game/SteppeGameMode.h"
#include "AI/SteppeHerdManager.h"
ASteppeRiderCharacter::ASteppeRiderCharacter()
{
    PrimaryActorTick.bCanEverTick=false;
    Riding=CreateDefaultSubobject<URidingComponent>(TEXT("Riding"));
    Lasso=CreateDefaultSubobject<ULassoComponent>(TEXT("Lasso"));
    Balance=CreateDefaultSubobject<URiderBalanceComponent>(TEXT("RiderBalance"));
    Balance->AddTickPrerequisiteComponent(Lasso);
    Feedback=CreateDefaultSubobject<USteppeFeedbackComponent>(TEXT("SteppeFeedback"));
    Feedback->AddTickPrerequisiteComponent(Lasso);
    Feedback->AddTickPrerequisiteComponent(Balance);
    CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent()); CameraBoom->TargetArmLength=350;
    CameraBoom->bUsePawnControlRotation=true; CameraBoom->bEnableCameraLag=true;
    CameraBoom->bUseCameraLagSubstepping=true; CameraBoom->TargetOffset=FVector(0,0,65);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName); Camera->bUsePawnControlRotation=false;
    RidingCamera=CreateDefaultSubobject<URidingCameraComponent>(TEXT("RidingCamera"));
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true; GetCharacterMovement()->MaxWalkSpeed=450;
    auto* Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderRider")); Body->SetupAttachment(GetRootComponent());
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision); Body->SetRelativeScale3D(FVector(.4f,.4f,1.2f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Shape.Succeeded()) { Body->SetStaticMesh(Shape.Object); }
}
void ASteppeRiderCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    auto* PC=Cast<APlayerController>(Controller);
    if (InputConfig && PC && PC->GetLocalPlayer())
    {
        if (auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (InputConfig->OnFoot) { Subsystem->RemoveMappingContext(InputConfig->OnFoot); }
            if (InputConfig->Riding) { Subsystem->RemoveMappingContext(InputConfig->Riding); }
        }
    }
    Super::EndPlay(Reason);
}
void ASteppeRiderCharacter::EnsureInputConfig()
{
    if (!InputConfig) { InputConfig=NewObject<USteppeInputConfig>(this); InputConfig->CreateRuntimeDefaults(); }
}
void ASteppeRiderCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    if (Controller) { Riding->AddTickPrerequisiteActor(Controller); }
    RefreshInputContext();
}
void ASteppeRiderCharacter::RefreshInputContext()
{
    EnsureInputConfig();
    auto* PC=Cast<APlayerController>(Controller); if (!PC || !PC->GetLocalPlayer()) { return; }
    if (auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        if (InputConfig->OnFoot) { Subsystem->RemoveMappingContext(InputConfig->OnFoot); }
        if (InputConfig->Riding) { Subsystem->RemoveMappingContext(InputConfig->Riding); }
        auto* Context=Riding->IsMounted()?InputConfig->Riding.Get():InputConfig->OnFoot.Get();
        if (Context) { Subsystem->AddMappingContext(Context,0); }
        else { UE_LOG(LogSteppe,Warning,TEXT("Input config missing active mapping context on %s."),*GetName()); }
    }
}
void ASteppeRiderCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input); EnsureInputConfig();
    auto* Enhanced=Cast<UEnhancedInputComponent>(Input); if (!Enhanced) { UE_LOG(LogSteppe,Error,TEXT("EnhancedInputComponent is required.")); return; }
    auto Axis=[this,Enhanced](UInputAction* Action,void (ASteppeRiderCharacter::*Callback)(const FInputActionValue&))
    {
        if (!Action) { UE_LOG(LogSteppe,Warning,TEXT("Missing input action; binding skipped.")); return; }
        Enhanced->BindAction(Action,ETriggerEvent::Triggered,this,Callback);
        Enhanced->BindAction(Action,ETriggerEvent::Completed,this,Callback);
        Enhanced->BindAction(Action,ETriggerEvent::Canceled,this,Callback);
    };
    Axis(InputConfig->Move,&ASteppeRiderCharacter::Move); Axis(InputConfig->Look,&ASteppeRiderCharacter::Look);
    Axis(InputConfig->Sprint,&ASteppeRiderCharacter::Sprint); Axis(InputConfig->Brake,&ASteppeRiderCharacter::Brake);
    if (InputConfig->MountDismount) { Enhanced->BindAction(InputConfig->MountDismount,ETriggerEvent::Started,this,&ASteppeRiderCharacter::Interact); }
    if (InputConfig->Interact) { Enhanced->BindAction(InputConfig->Interact,ETriggerEvent::Started,this,&ASteppeRiderCharacter::Interact); }
    if (InputConfig->RestartTrial) { Enhanced->BindAction(InputConfig->RestartTrial,ETriggerEvent::Started,this,&ASteppeRiderCharacter::RestartTrial); }
    if (InputConfig->FocusTarget) { Enhanced->BindAction(InputConfig->FocusTarget,ETriggerEvent::Started,this,&ASteppeRiderCharacter::FocusTarget); }
    if (InputConfig->AimLasso)
    {
        Enhanced->BindAction(InputConfig->AimLasso,ETriggerEvent::Started,this,&ASteppeRiderCharacter::BeginLassoAim);
        Enhanced->BindAction(InputConfig->AimLasso,ETriggerEvent::Completed,this,&ASteppeRiderCharacter::EndLassoAim);
        Enhanced->BindAction(InputConfig->AimLasso,ETriggerEvent::Canceled,this,&ASteppeRiderCharacter::EndLassoAim);
    }
    if (InputConfig->ThrowLasso) { Enhanced->BindAction(InputConfig->ThrowLasso,ETriggerEvent::Started,this,&ASteppeRiderCharacter::ThrowLasso); }
    Axis(InputConfig->BraceLasso,&ASteppeRiderCharacter::BraceLasso);
    if (InputConfig->CaptureHorse) { Enhanced->BindAction(InputConfig->CaptureHorse,ETriggerEvent::Started,this,&ASteppeRiderCharacter::CaptureHorse); }
    if (InputConfig->Debug) { Enhanced->BindAction(InputConfig->Debug,ETriggerEvent::Started,this,&ASteppeRiderCharacter::ToggleDebug); }
    RefreshInputContext();
}
void ASteppeRiderCharacter::ResetRidingInput() { Intent.Reset(); Riding->SetIntent(Intent); }
FGameplayTag ASteppeRiderCharacter::GetRiderStateTag() const
{
    if (Balance)
    {
        if (Balance->State==ERiderBalanceState::Dragged) { return SteppeTags::Rider_State_Dragged; }
        if (Balance->State==ERiderBalanceState::Recovering) { return SteppeTags::Rider_State_Recovering; }
        if (Balance->State==ERiderBalanceState::Warning) { return SteppeTags::Rider_State_BalanceWarning; }
        if (Balance->State==ERiderBalanceState::Falling) { return SteppeTags::Rider_State_Falling; }
    }
    if (Riding->IsMounted()) { return SteppeTags::Rider_State_Mounted; }
    return GetCharacterMovement()->IsFalling()?SteppeTags::Rider_State_Falling:SteppeTags::Rider_State_OnFoot;
}
void ASteppeRiderCharacter::Move(const FInputActionValue& Value)
{
    if (Balance && (Balance->State==ERiderBalanceState::Dragged || Balance->State==ERiderBalanceState::Falling)) { return; }
    const FVector2D Axis=Value.Get<FVector2D>(); Intent.Forward=Axis.Y; Intent.Turn=Axis.X; Intent.Clamp(); Riding->SetIntent(Intent);
    if (!Riding->IsMounted() && Controller)
    {
        const FRotator Yaw(0,Controller->GetControlRotation().Yaw,0);
        AddMovementInput(Yaw.Vector(),Axis.Y); AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y),Axis.X);
    }
}
void ASteppeRiderCharacter::Look(const FInputActionValue& Value)
{
    Intent.LookInput=Value.Get<FVector2D>(); Riding->SetIntent(Intent);
    AddControllerYawInput(Intent.LookInput.X*LookSensitivity); AddControllerPitchInput(Intent.LookInput.Y*LookSensitivity);
}
void ASteppeRiderCharacter::Sprint(const FInputActionValue& Value) { Intent.bSprint=Value.Get<bool>(); Riding->SetIntent(Intent); }
void ASteppeRiderCharacter::Brake(const FInputActionValue& Value) { Intent.bBrake=Value.Get<bool>(); Riding->SetIntent(Intent); }
void ASteppeRiderCharacter::Interact()
{
    if (Riding->IsMounted()) { Riding->Dismount(); return; }
    if (auto* Mode=GetWorld()->GetAuthGameMode<ASteppeGameMode>())
    {
        if (Mode->HerdManager && Mode->HerdManager->HandleFirstContactInteraction(this)) { return; }
    }
    ASteppeHorseCharacter* Closest=nullptr; float Distance=Riding->MountDistance;
    // On interaction only; never scan all horses per frame.
    for (TActorIterator<ASteppeHorseCharacter> It(GetWorld()); It; ++It)
    {
        const float D=FVector::Dist(It->GetActorLocation(),GetActorLocation());
        if (D<Distance && It->bCanBeMounted && !It->MountedRider.IsValid()) { Closest=*It; Distance=D; }
    }
    Riding->TryMount(Closest);
}
void ASteppeRiderCharacter::ToggleDebug() { if (auto* PC=Cast<ASteppePlayerController>(Controller)) { PC->SteppeToggleDebug(); } }
void ASteppeRiderCharacter::FocusTarget() { if (auto* PC=Cast<ASteppePlayerController>(Controller)) { PC->SteppeFocusTarget(); } }
void ASteppeRiderCharacter::BeginLassoAim() { Lasso->BeginAim(); }
void ASteppeRiderCharacter::EndLassoAim() { Lasso->CancelAim(); }
void ASteppeRiderCharacter::ThrowLasso() { if (Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Subdued || Lasso->State==ELassoState::Captured) { Lasso->Release(); } else { Lasso->Throw(); } }
void ASteppeRiderCharacter::BraceLasso(const FInputActionValue& Value) { Lasso->SetBracing(Value.Get<bool>()); }
void ASteppeRiderCharacter::CaptureHorse() { Lasso->Capture(); }
void ASteppeRiderCharacter::RestartTrial() { if (auto* PC=Cast<ASteppePlayerController>(Controller)) { PC->SteppeRestartTrial(); } }
