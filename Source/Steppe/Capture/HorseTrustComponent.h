#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HorseTrustComponent.generated.h"

class ASteppeRiderCharacter;

UENUM(BlueprintType)
enum class EPostCaptureState : uint8 { Inactive, Secured, CalmApproach, Rejected, ReadyForContact, FirstContact, Leading, Delivered, Named };

UCLASS(ClassGroup=(Steppe), meta=(BlueprintSpawnableComponent))
class STEPPE_API UHorseTrustComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHorseTrustComponent();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

    void BeginSecured(ASteppeRiderCharacter* Rider);
    void AdvanceApproach(float DeltaSeconds, float Distance, float ApproachSpeed, bool bRiderMounted);
    bool TryFirstContact(ASteppeRiderCharacter* Rider);
    bool BeginLeading(ASteppeRiderCharacter* Rider);
    bool MarkDelivered(ASteppeRiderCharacter* Rider);
    bool ConfirmName(const FString& NewName);
    void InitializeIdentity(int32 MemberIndex);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="100")) float AwarenessRadius = 800.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="0")) float SafeApproachSpeed = 120.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="0")) float RushApproachSpeed = 300.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="50")) float ContactDistance = 220.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="0.1")) float CalmHoldSeconds = 2.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="0")) float RejectionCooldown = 1.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Approach", meta=(ClampMin="0")) float RetreatSpeed = 180.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Trust", meta=(ClampMin="0", ClampMax="100")) float FirstContactTrust = 10.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Approach") EPostCaptureState State = EPostCaptureState::Inactive;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Approach") float DistanceToRider = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Approach") float RiderApproachSpeed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Approach") float Pressure = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Approach") float CalmProgress = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Trust") float Trust = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Trust") bool bFirstContact = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lead") bool bLeading = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lead") bool bDelivered = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") bool bNamed = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") FString HorseId = TEXT("H1");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") FString HorseName;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") FString Sex = TEXT("Mare");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") int32 AgeYears = 5;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") FString Coat = TEXT("Bay");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Identity") FString Temperament = TEXT("Steady");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lead") FVector LeadStartLocation = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Approach") FString Feedback = TEXT("Capture the horse first");

private:
    UPROPERTY() TWeakObjectPtr<ASteppeRiderCharacter> Interactor;
    float RejectionRemaining = 0.f;
};
