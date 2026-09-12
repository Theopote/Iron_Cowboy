#include "Debug/SteppeHUD.h"
#include "Debug/SteppeDebugSubsystem.h"
#include "Game/SteppeGameMode.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"
#include "Character/Horse/HorseAttributeComponent.h"
#include "Character/Horse/HorseLocomotionConfig.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Character/Horse/SteppeWildHorseCharacter.h"
#include "AI/HorseBrainComponent.h"
#include "AI/SteppeHerdManager.h"
#include "GameFramework/PlayerController.h"
void ASteppeHUD::DrawHUD()
{
    Super::DrawHUD();
    DrawText(TEXT("STEPPE | W/S urge/slow  A/D reins  Mouse free look  Shift sprint  Ctrl brake  E mount  Q target  F1 telemetry  F2 retry"),FLinearColor::White,24,20,nullptr,1.f);
    auto* Debug=GetWorld()->GetSubsystem<USteppeDebugSubsystem>();
    if (!Debug || (!Debug->IsHorseDebugEnabled() && !Debug->IsMovementDebugEnabled())) { return; }
    auto* Mode=GetWorld()->GetAuthGameMode<ASteppeGameMode>();
    auto* Horse=Mode?Mode->PlaygroundHorse.Get():nullptr; if (!IsValid(Horse)) { return; }
    auto* Move=Cast<UHorseMovementComponent>(Horse->GetCharacterMovement()); if (!Move) { return; }
    const auto& C=*Horse->GetLocomotionConfig();
    if (IsValid(Mode->WildHorse))
    {
        auto* Wild = Mode->WildHorse.Get();
        auto* Brain = Wild->Brain.Get();
        if (Debug->IsHorseDebugEnabled())
        {
            const auto* Herd=Mode->HerdManager.Get();
            const int32 TargetIndex=Herd?Mode->WildHorses.IndexOfByKey(Herd->FocusedHorse):-1;
            const FString TargetLine=Herd && Herd->FocusedHorse
                ?FString::Printf(TEXT("TARGET: H%d | separation %.1f m | isolate %.0f%%%s"),TargetIndex+1,
                    Herd->IsolationDistance/100.f,Herd->IsolationProgress*100.f,Herd->bTargetIsolated?TEXT(" | ISOLATED"):TEXT(""))
                :TEXT("TARGET: none | look toward a horse and press Q");
            DrawRect(FLinearColor(0,0,0,.65f),18,250,650,138);
            DrawText(FString::Printf(TEXT("WILD HERD: %d horses | alarm sources %d\n%s\nFOCUS: %s | awareness %.0f%% | neighbors %d\nDistance %.1f m | approach %.1f m/s | visible %s\nPath %s | cut the target away from the herd."),
                Mode->WildHorses.Num(),Mode->HerdManager?Mode->HerdManager->AlarmSourceCount:0,
                *TargetLine,
                *UEnum::GetDisplayValueAsText(Brain->State).ToString(), Brain->Awareness*100.f,
                Brain->HerdNeighborCount,
                SteppeUnits::ToMetersPerSecond(Brain->ThreatDistance), SteppeUnits::ToMetersPerSecond(Brain->ApproachSpeed),
                Brain->bThreatVisible?TEXT("yes"):TEXT("no"),Brain->bBrakingForHazard?TEXT("hazard braking"):(Brain->bPathBlocked?TEXT("blocked"):TEXT("clear"))),
                FLinearColor(1,.85f,.3f),26,257,nullptr,1.f);
            for (int32 Index=0; Index<Mode->WildHorses.Num(); ++Index)
            {
                const auto* Member=Mode->WildHorses[Index].Get();
                if (!Member) { continue; }
                FVector2D LabelPosition;
                if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(Member->GetActorLocation()+FVector(0,0,180),LabelPosition)
                    && (LabelPosition.X>680.f || LabelPosition.Y>370.f))
                {
                    const auto MemberState=Member->Brain->State;
                    const bool bTarget=Mode->HerdManager && Mode->HerdManager->FocusedHorse==Member;
                    const FLinearColor Color=bTarget?FLinearColor(0,1,1):MemberState==EWildHorseState::Fleeing?FLinearColor(1,.25f,.1f):
                        (MemberState==EWildHorseState::Alert?FLinearColor::Yellow:FLinearColor(.55f,.8f,1.f));
                    DrawText(FString::Printf(TEXT("%sH%d %s"),bTarget?TEXT("TARGET "):TEXT(""),Index+1,*UEnum::GetDisplayValueAsText(MemberState).ToString()),
                        Color,LabelPosition.X,LabelPosition.Y,nullptr,.9f);
                }
            }
        }
        if (Debug->IsMovementDebugEnabled())
        {
            const FVector Start = Wild->GetActorLocation()+FVector(0,0,130);
            DrawDebugDirectionalArrow(GetWorld(), Start, Start+Brain->SteeringDirection*500.f,40,FColor::Orange,false,0,0,4);
            DrawDebugSphere(GetWorld(), Brain->Goal,90,12,FColor::Orange,false,0,0,2);
            if (Mode->HerdManager)
            {
                DrawDebugSphere(GetWorld(),Mode->HerdManager->HerdCenter,90,12,FColor::Purple,false,0,0,4);
                DrawDebugDirectionalArrow(GetWorld(),Mode->HerdManager->HerdCenter,
                    Mode->HerdManager->HerdCenter+Mode->HerdManager->AverageVelocity.GetSafeNormal2D()*500.f,
                    50,FColor::Purple,false,0,0,4);
                if (Mode->HerdManager->FocusedHorse)
                {
                    const FVector Target=Mode->HerdManager->FocusedHorse->GetActorLocation()+FVector(0,0,120);
                    const FVector Rest=Mode->HerdManager->RestHerdCenter+FVector(0,0,120);
                    DrawDebugSphere(GetWorld(),Target,140,16,FColor::Cyan,false,0,0,5);
                    DrawDebugLine(GetWorld(),Target,Rest,FColor::Cyan,false,0,0,5);
                }
            }
        }
    }
    const TCHAR* Stress=Move->TurnStress>=C.StressCritical?TEXT("CRITICAL"):(Move->TurnStress>=C.StressWarning?TEXT("WARNING"):TEXT("SAFE"));
    if (Debug->IsHorseDebugEnabled())
    {
        const FString Info=FString::Printf(TEXT("State: %s   Gait: %s\nSpeed: %.1f cm/s | %.2f m/s | %.1f km/h\nDesired: %.1f cm/s   Accel: %.1f cm/s2\nRider forward: %.2f  turn: %.2f | horse turn: %.2f\nTurn rate: %.1f deg/s  Stress: %.2f %s\nStamina: %.1f / %.1f (%.0f%%)\nRider: %s | Slope: %.1f deg\nE dismount requires speed <= 200 cm/s and clear ground"),
            *UEnum::GetValueAsString(Move->HorseState),*UEnum::GetValueAsString(Move->Gait),Move->CurrentSpeed,
            SteppeUnits::ToMetersPerSecond(Move->CurrentSpeed),SteppeUnits::ToKilometersPerHour(Move->CurrentSpeed),Move->DesiredSpeed,Move->ActualAcceleration,
            Move->RiderIntent.Forward,Move->RiderIntent.Turn,Move->HorseIntent.DesiredTurn,Move->EffectiveTurnRate,Move->TurnStress,Stress,
            Horse->Attributes->CurrentStamina,Horse->Attributes->MaxStamina,Horse->Attributes->GetStaminaNormalized()*100,
            *GetNameSafe(Horse->MountedRider.Get()),Move->GroundSlope);
        DrawRect(FLinearColor(0,0,0,.65f),18,48,650,195);
        DrawText(Info,FLinearColor(.85f,1,.8f),26,55,nullptr,1.1f);
    }
    if (Debug->IsMovementDebugEnabled())
    {
        const FVector Origin=Horse->GetActorLocation()+FVector(0,0,110);
        DrawDebugDirectionalArrow(GetWorld(),Origin,Origin+Horse->GetActorForwardVector()*250,35,FColor::Green,false,0,0,3);
        DrawDebugDirectionalArrow(GetWorld(),Origin,Origin+Horse->GetVelocity()*.3f,35,FColor::Cyan,false,0,0,3);
        const FVector Heading=FRotator(0,Move->DesiredHeading,0).Vector();
        DrawDebugDirectionalArrow(GetWorld(),Origin,Origin+Heading*300,35,FColor::Yellow,false,0,0,3);
        DrawDebugDirectionalArrow(GetWorld(),Origin+FVector(0,0,15),Origin+FVector(0,0,15)+Heading*Move->DesiredSpeed*.3f,35,FColor::Magenta,false,0,0,3);
    }
}
