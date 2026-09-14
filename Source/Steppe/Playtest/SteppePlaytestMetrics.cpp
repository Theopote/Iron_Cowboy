#include "Playtest/SteppePlaytestMetrics.h"
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "AI/SteppeHerdManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Steppe.h"

void FSteppePlaytestRound::RecordLassoTransition(ELassoState Previous,ELassoState Current,const FString& Feedback,float Tension,ELassoHitZone Zone)
{
    PeakTension=FMath::Max(PeakTension,Tension);
    if (Previous==Current) { return; }
    if (Current==ELassoState::Thrown) { ++ThrowCount; }
    if (Current==ELassoState::Attached)
    {
        if (Previous!=ELassoState::Thrown) { ++ThrowCount; }
        ++AttachCount;
        if (HitZone.IsEmpty()) { HitZone=UEnum::GetDisplayValueAsText(Zone).ToString(); }
    }
    if (Current==ELassoState::Recovering)
    {
        if (Feedback.Contains(TEXT("broke"),ESearchCase::IgnoreCase)) { ++RopeBreakCount; }
        else if (Feedback.Contains(TEXT("Missed"),ESearchCase::IgnoreCase) || Feedback.Contains(TEXT("blocked"),ESearchCase::IgnoreCase) || Feedback.Contains(TEXT("lost"),ESearchCase::IgnoreCase)) { ++MissCount; }
        else { ++ReleaseCount; }
    }
}

void FSteppePlaytestRound::RecordBalanceTransition(ERiderBalanceState Previous,ERiderBalanceState Current,float Balance,float FallThreshold)
{
    PeakBalanceRisk=FMath::Max(PeakBalanceRisk,FallThreshold>0.f?Balance/FallThreshold:0.f);
    if (Previous==Current) { return; }
    if (Current==ERiderBalanceState::Warning) { ++BalanceWarningCount; }
    else if (Current==ERiderBalanceState::Falling) { ++FallCount; }
    else if (Current==ERiderBalanceState::Dragged) { ++DraggedCount; }
}

USteppePlaytestMetricsComponent::USteppePlaytestMetricsComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
}

void USteppePlaytestMetricsComponent::BeginRound(ASteppeRiderCharacter* InRider,ASteppeHerdManager* InHerd,const FSteppeTrialProgress& Trial)
{
    Rider=InRider;
    Herd=InHerd;
    Round=FSteppePlaytestRound();
    const FDateTime Now=FDateTime::UtcNow();
    Round.StartedUtc=Now.ToIso8601();
    Round.SessionId=Now.ToString(TEXT("%Y%m%d-%H%M%S-%s"));
    Round.bAutomatedSession=FParse::Param(FCommandLine::Get(),TEXT("SteppeSmoke"));
    Round.ElapsedSeconds=Trial.ElapsedSeconds;
    PreviousLassoState=InRider && InRider->Lasso?InRider->Lasso->State:ELassoState::Stored;
    PreviousBalanceState=InRider && InRider->Balance?InRider->Balance->State:ERiderBalanceState::Stable;
    bDangerousTension=false;
    bFinalized=false;
    LastWrittenFile.Reset();
}

void USteppePlaytestMetricsComponent::Observe(const FSteppeTrialProgress& Trial)
{
    if (bFinalized || !Rider.IsValid()) { return; }
    auto* CurrentRider=Rider.Get();
    auto* CurrentHerd=Herd.Get();
    Round.ElapsedSeconds=Trial.ElapsedSeconds;
    Round.Score=Trial.Score;
    if (CurrentHerd && IsValid(CurrentHerd->FocusedHorse) && Round.TargetSelectedSeconds<0.f)
    {
        Round.TargetSelectedSeconds=Trial.ElapsedSeconds;
        Round.TargetHorse=CurrentHerd->FocusedHorse->GetName();
        Round.TargetArchetype=CurrentHerd->FocusedHorse->ArchetypeLabel;
    }
    if (CurrentHerd && CurrentHerd->bTargetIsolated && Round.IsolatedSeconds<0.f) { Round.IsolatedSeconds=Trial.ElapsedSeconds; }
    if (CurrentHerd && CurrentHerd->CapturedCount>0 && Round.CapturedSeconds<0.f) { Round.CapturedSeconds=Trial.ElapsedSeconds; }
    if (CurrentHerd && CurrentHerd->FirstContactCount>0 && Round.FirstContactSeconds<0.f) { Round.FirstContactSeconds=Trial.ElapsedSeconds; }
    if (CurrentHerd && CurrentHerd->DeliveredCount>0 && Round.DeliveredSeconds<0.f) { Round.DeliveredSeconds=Trial.ElapsedSeconds; }
    if (CurrentHerd && CurrentHerd->NamedCount>0 && Round.NamedSeconds<0.f) { Round.NamedSeconds=Trial.ElapsedSeconds; }
    if (CurrentRider->Lasso)
    {
        Round.RecordLassoTransition(PreviousLassoState,CurrentRider->Lasso->State,CurrentRider->Lasso->Feedback,CurrentRider->Lasso->Tension,CurrentRider->Lasso->HitZone);
        const bool bNowDangerous=CurrentRider->Lasso->Tension>CurrentRider->Lasso->UsefulTensionMax;
        if (bNowDangerous && !bDangerousTension) { ++Round.DangerousTensionCount; }
        bDangerousTension=bNowDangerous;
        PreviousLassoState=CurrentRider->Lasso->State;
    }
    if (CurrentRider->Balance)
    {
        Round.RecordBalanceTransition(PreviousBalanceState,CurrentRider->Balance->State,CurrentRider->Balance->Balance,CurrentRider->Balance->FallThreshold);
        PreviousBalanceState=CurrentRider->Balance->State;
    }
    if (Trial.State==ESteppeTrialState::Success) { Finish(TEXT("Success"),TEXT("NamedRequiredHorses"),&Trial); }
    else if (Trial.State==ESteppeTrialState::Failed) { Finish(TEXT("Failed"),TEXT("TimeExpired"),&Trial); }
}

void USteppePlaytestMetricsComponent::FinalizeForRestart()
{
    if (!bFinalized) { ++Round.RetryCount; Finish(TEXT("Abandoned"),TEXT("Restarted"),nullptr); }
}

void USteppePlaytestMetricsComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (!bFinalized) { Finish(TEXT("Abandoned"),TEXT("WorldEnded"),nullptr); }
    Super::EndPlay(Reason);
}

void USteppePlaytestMetricsComponent::Finish(const TCHAR* Result,const TCHAR* Reason,const FSteppeTrialProgress* Trial)
{
    if (bFinalized) { return; }
    if (Trial) { Round.ElapsedSeconds=Trial->ElapsedSeconds; Round.Score=Trial->Score; }
    Round.Result=Result;
    Round.EndReason=Reason;
    bFinalized=true;
    WriteJson();
    UE_LOG(LogSteppe,Display,TEXT("STEPPE_P14_METRICS: Result=%s Reason=%s Time=%.1f Throws=%d Attach=%d Miss=%d Break=%d Danger=%d Warn=%d Fall=%d Drag=%d Score=%d File=%s"),
        *Round.Result,*Round.EndReason,Round.ElapsedSeconds,Round.ThrowCount,Round.AttachCount,Round.MissCount,Round.RopeBreakCount,
        Round.DangerousTensionCount,Round.BalanceWarningCount,Round.FallCount,Round.DraggedCount,Round.Score,*LastWrittenFile);
}

bool USteppePlaytestMetricsComponent::WriteJson()
{
    const TSharedRef<FJsonObject> Root=MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("schemaVersion"),TEXT("1.1"));
    Root->SetStringField(TEXT("sessionId"),Round.SessionId);
    Root->SetStringField(TEXT("startedUtc"),Round.StartedUtc);
    Root->SetBoolField(TEXT("isAutomated"),Round.bAutomatedSession);
    Root->SetStringField(TEXT("result"),Round.Result);
    Root->SetStringField(TEXT("endReason"),Round.EndReason);
    Root->SetStringField(TEXT("targetHorse"),Round.TargetHorse);
    Root->SetStringField(TEXT("targetArchetype"),Round.TargetArchetype);
    Root->SetStringField(TEXT("hitZone"),Round.HitZone);
    Root->SetNumberField(TEXT("elapsedSeconds"),Round.ElapsedSeconds);
    Root->SetNumberField(TEXT("score"),Round.Score);
    const TSharedRef<FJsonObject> Stages=MakeShared<FJsonObject>();
    Stages->SetNumberField(TEXT("targetSelected"),Round.TargetSelectedSeconds);
    Stages->SetNumberField(TEXT("isolated"),Round.IsolatedSeconds);
    Stages->SetNumberField(TEXT("captured"),Round.CapturedSeconds);
    Stages->SetNumberField(TEXT("firstContact"),Round.FirstContactSeconds);
    Stages->SetNumberField(TEXT("delivered"),Round.DeliveredSeconds);
    Stages->SetNumberField(TEXT("named"),Round.NamedSeconds);
    Root->SetObjectField(TEXT("stageSeconds"),Stages);
    const TSharedRef<FJsonObject> Actions=MakeShared<FJsonObject>();
    Actions->SetNumberField(TEXT("throws"),Round.ThrowCount);
    Actions->SetNumberField(TEXT("attachments"),Round.AttachCount);
    Actions->SetNumberField(TEXT("misses"),Round.MissCount);
    Actions->SetNumberField(TEXT("ropeBreaks"),Round.RopeBreakCount);
    Actions->SetNumberField(TEXT("releases"),Round.ReleaseCount);
    Actions->SetNumberField(TEXT("retries"),Round.RetryCount);
    Root->SetObjectField(TEXT("actions"),Actions);
    const TSharedRef<FJsonObject> Risk=MakeShared<FJsonObject>();
    Risk->SetNumberField(TEXT("dangerousTensionEntries"),Round.DangerousTensionCount);
    Risk->SetNumberField(TEXT("balanceWarnings"),Round.BalanceWarningCount);
    Risk->SetNumberField(TEXT("falls"),Round.FallCount);
    Risk->SetNumberField(TEXT("dragged"),Round.DraggedCount);
    Risk->SetNumberField(TEXT("peakTension"),Round.PeakTension);
    Risk->SetNumberField(TEXT("peakBalanceRisk"),Round.PeakBalanceRisk);
    Root->SetObjectField(TEXT("risk"),Risk);
    FString Text;
    const TSharedRef<TJsonWriter<>> Writer=TJsonWriterFactory<>::Create(&Text);
    if (!FJsonSerializer::Serialize(Root,Writer)) { return false; }
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Playtests");
    IPlatformFile& PlatformFile=FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*Folder);
    const bool bMetricsSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeMetricsSmoke"));
    const bool bFailureSmoke=FParse::Param(FCommandLine::Get(),TEXT("SteppeMetricsFailureSmoke"));
    const FString Name=bMetricsSmoke?(bFailureSmoke?TEXT("P14-Failure.json"):TEXT("P14-Success.json")):FString::Printf(TEXT("Round-%s.json"),*Round.SessionId);
    LastWrittenFile=Folder/Name;
    return FFileHelper::SaveStringToFile(Text,*LastWrittenFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString USteppePlaytestMetricsComponent::GetCompactSummary() const
{
    return FString::Printf(TEXT("%s | %.1fs | %d throws | %d misses | %d risks | %d pts"),*Round.Result,Round.ElapsedSeconds,
        Round.ThrowCount,Round.MissCount+Round.RopeBreakCount,Round.DangerousTensionCount+Round.BalanceWarningCount+Round.FallCount,Round.Score);
}
