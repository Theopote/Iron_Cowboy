#pragma once

#include "CoreMinimal.h"
#include "SteppeTrialState.generated.h"

UENUM(BlueprintType)
enum class ESteppeTrialState : uint8 { NotStarted, Running, Success, Failed };

USTRUCT(BlueprintType)
struct STEPPE_API FSteppeTrialProgress
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) ESteppeTrialState State = ESteppeTrialState::NotStarted;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ElapsedSeconds = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RemainingSeconds = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Captured = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 FirstContacts = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Delivered = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Named = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 RequiredCaptures = 1;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Score = 0;
    float DurationSeconds = 120.f;

    void Start(float Duration, int32 Required)
    {
        DurationSeconds=FMath::Max(1.f,Duration);
        RequiredCaptures=FMath::Max(1,Required);
        State=ESteppeTrialState::Running;
        ElapsedSeconds=0.f;
        RemainingSeconds=DurationSeconds;
        Captured=0;
        FirstContacts=0;
        Delivered=0;
        Named=0;
        Score=0;
    }

    void Advance(float DeltaSeconds, int32 CapturedCount, int32 FirstContactCount, int32 DeliveredCount, int32 NamedCount)
    {
        if (State!=ESteppeTrialState::Running) { return; }
        Captured=FMath::Max(0,CapturedCount);
        FirstContacts=FMath::Max(0,FirstContactCount);
        Delivered=FMath::Max(0,DeliveredCount);
        Named=FMath::Max(0,NamedCount);
        ElapsedSeconds=FMath::Min(DurationSeconds,ElapsedSeconds+FMath::Max(0.f,DeltaSeconds));
        RemainingSeconds=FMath::Max(0.f,DurationSeconds-ElapsedSeconds);
        if (Named>=RequiredCaptures)
        {
            State=ESteppeTrialState::Success;
            Score=Captured*1000+FMath::CeilToInt(RemainingSeconds)*10;
        }
        else if (RemainingSeconds<=0.f) { State=ESteppeTrialState::Failed; }
    }
};
