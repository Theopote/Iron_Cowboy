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
#include "GameFramework/PlayerController.h"
void ASteppeHUD::DrawHUD()
{
    Super::DrawHUD();
    DrawText(TEXT("STEPPE | W/S urge/slow  A/D reins  Mouse free look  Shift sprint  Ctrl brake  E mount  F1 telemetry  F2 retry"),FLinearColor::White,24,20,nullptr,1.f);
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
            DrawRect(FLinearColor(0,0,0,.65f),18,250,650,90);
            DrawText(FString::Printf(TEXT("WILD HORSE: %s | awareness %.0f%%\nDistance %.1f m | closing %.1f m/s | visible %s\nPath %s | approach slowly, then compare a fast chase. Wild horse cannot be mounted."),
                *UEnum::GetDisplayValueAsText(Brain->State).ToString(), Brain->Awareness*100.f,
                SteppeUnits::ToMetersPerSecond(Brain->ThreatDistance), SteppeUnits::ToMetersPerSecond(Brain->ClosingSpeed),
                Brain->bThreatVisible?TEXT("yes"):TEXT("no"),Brain->bBrakingForHazard?TEXT("hazard braking"):(Brain->bPathBlocked?TEXT("blocked"):TEXT("clear"))),
                FLinearColor(1,.85f,.3f),26,257,nullptr,1.f);
            FVector2D LabelPosition;
            if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(Wild->GetActorLocation()+FVector(0,0,180),LabelPosition)
                && (LabelPosition.X>680.f || LabelPosition.Y>350.f))
            {
                DrawText(TEXT("WILD HORSE"), FLinearColor::Yellow,LabelPosition.X,LabelPosition.Y,nullptr,1.f);
            }
        }
        if (Debug->IsMovementDebugEnabled())
        {
            const FVector Start = Wild->GetActorLocation()+FVector(0,0,130);
            DrawDebugDirectionalArrow(GetWorld(), Start, Start+Brain->SteeringDirection*500.f,40,FColor::Orange,false,0,0,4);
            DrawDebugSphere(GetWorld(), Brain->Goal,90,12,FColor::Orange,false,0,0,2);
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
