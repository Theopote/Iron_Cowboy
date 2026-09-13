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
#include "Character/Rider/SteppeRiderCharacter.h"
#include "Lasso/LassoComponent.h"
#include "Capture/HorseTrustComponent.h"
#include "Engine/Canvas.h"
#include "Camp/SteppeDeliveryZone.h"
void ASteppeHUD::DrawHUD()
{
    Super::DrawHUD();
    DrawText(TEXT("STEPPE | W/S urge/slow  A/D reins  Mouse look  Shift sprint  Ctrl brake  E mount  Q target  RMB aim  LMB throw/release  Space brace  C capture  F1/F2"),FLinearColor::White,24,20,nullptr,.85f);
    auto* Rider=PlayerOwner?Cast<ASteppeRiderCharacter>(PlayerOwner->GetPawn()):nullptr;
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    if (Lasso)
    {
        const FString LassoText=FString::Printf(TEXT("LASSO %s | %s"),*UEnum::GetDisplayValueAsText(Lasso->State).ToString(),*Lasso->Feedback);
        const FLinearColor LassoColor=(Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Captured)?FLinearColor(.35f,1.f,.35f):
            (Lasso->State==ELassoState::Thrown?FLinearColor(1,.55f,.1f):FLinearColor(.5f,1.f,1.f));
        const float LassoX=FMath::Max(18.f,Canvas->ClipX-570.f);
        DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,44,558,25);
        DrawText(LassoText,LassoColor,LassoX,48,nullptr,.95f);
        if (Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Subdued || Lasso->State==ELassoState::Captured)
        {
            const bool bCaptureComplete=Lasso->State==ELassoState::Captured;
            const bool bGreen=bCaptureComplete || (Lasso->Tension>=Lasso->UsefulTensionMin && Lasso->Tension<=Lasso->UsefulTensionMax);
            const FLinearColor TensionColor=bGreen?FLinearColor(.3f,1.f,.3f):FLinearColor(1.f,.3f,.15f);
            DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,73,558,43);
            const FString FightText=bCaptureComplete?TEXT("CAPTURE COMPLETE | LMB stow lasso"):
                FString::Printf(TEXT("TENSION %.0f%% %s | CONTROL %.0f%%"),Lasso->Tension*100.f,
                    bGreen?TEXT("STEADY"):TEXT("ADJUST"),Lasso->ControlProgress*100.f);
            DrawText(FightText,TensionColor,LassoX,78,nullptr,1.f);
            DrawRect(FLinearColor(.12f,.12f,.12f,1),LassoX,99,520,8);
            DrawRect(FLinearColor(.3f,.8f,1.f,1),LassoX,99,520*Lasso->ControlProgress,8);
        }
        if (Lasso->State==ELassoState::Aiming) { DrawText(TEXT("+"),FLinearColor::White,Canvas->ClipX*.5f-5,Canvas->ClipY*.5f-12,nullptr,1.5f); }
        if (Lasso->State==ELassoState::Thrown || Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Subdued || Lasso->State==ELassoState::Captured)
        {
            DrawDebugLine(GetWorld(),Lasso->RopeStart,Lasso->LoopLocation,FColor::Orange,false,0,0,6);
            if (Lasso->State!=ELassoState::Captured)
            {
                DrawDebugSphere(GetWorld(),Lasso->LoopLocation,Lasso->CaptureRadius,16,FColor::Yellow,false,0,0,4);
            }
        }
    }
    auto* Mode=GetWorld()->GetAuthGameMode<ASteppeGameMode>();
    if (Mode && Mode->bEnableTrial && Mode->Trial.State!=ESteppeTrialState::NotStarted)
    {
        const int32 Seconds=FMath::CeilToInt(Mode->Trial.RemainingSeconds);
        const FString Mission=FString::Printf(TEXT("MISSION  Secure %d/%d  Contact %d/%d  Deliver %d/%d  Name %d/%d  |  %02d:%02d  |  SCORE %d"),
            Mode->Trial.Captured,Mode->Trial.RequiredCaptures,Mode->Trial.FirstContacts,Mode->Trial.RequiredCaptures,
            Mode->Trial.Delivered,Mode->Trial.RequiredCaptures,Mode->Trial.Named,Mode->Trial.RequiredCaptures,
            Seconds/60,Seconds%60,Mode->Trial.Score);
        const float MissionY=Canvas->ClipY-42.f;
        const bool bUrgent=Mode->Trial.State==ESteppeTrialState::Running && Mode->Trial.RemainingSeconds<=30.f;
        const float Pulse=bUrgent && Mode->Trial.RemainingSeconds<=10.f ? .65f+.35f*FMath::Sin(Mode->Trial.ElapsedSeconds*8.f) : 1.f;
        const FLinearColor MissionColor=bUrgent?FLinearColor(1.f,.2f,.1f,Pulse):FLinearColor(1.f,.88f,.25f);
        DrawRect(FLinearColor(0,0,0,.72f),18,MissionY-6,760,32);
        DrawText(Mission,MissionColor,26,MissionY,nullptr,1.05f);
        if (Mode->Trial.State==ESteppeTrialState::Running)
        {
            FString Objective;
            UHorseTrustComponent* PendingTrust=nullptr;
            if (Mode->HerdManager)
            {
                for (const TObjectPtr<ASteppeWildHorseCharacter>& Horse : Mode->HerdManager->CapturedHorses)
                {
                    if (Horse && Horse->Trust && !Horse->Trust->bNamed) { PendingTrust=Horse->Trust; break; }
                }
            }
            if (PendingTrust)
            {
                if (PendingTrust->State==EPostCaptureState::Leading && Mode->DeliveryZone)
                {
                    const float CampDistance=Rider?FVector::Dist2D(Rider->GetActorLocation(),Mode->DeliveryZone->GetActorLocation())/100.f:0.f;
                    const auto* LeadHorse=Cast<ASteppeWildHorseCharacter>(PendingTrust->GetOwner());
                    if (LeadHorse && LeadHorse->Brain->LeadDistance>LeadHorse->Brain->LeadMaxDistance)
                    {
                        Objective=TEXT("WAIT  The horse lost pace - go back and let it catch up");
                    }
                    else { Objective=FString::Printf(TEXT("NEXT  Lead the horse to CAMP / PEN  |  %.1f m"),CampDistance); }
                }
                else if (PendingTrust->State==EPostCaptureState::FirstContact)
                {
                    Objective=TEXT("NEXT  Press E again to take the lead rope");
                }
                else if (PendingTrust->State==EPostCaptureState::Delivered)
                {
                    Objective=TEXT("NEXT  Name the horse on the Horse Card");
                }
                else
                {
                    Objective=FString::Printf(TEXT("NEXT  %s  |  CALM %.0f%%"),*PendingTrust->Feedback,PendingTrust->CalmProgress*100.f);
                }
            }
            else if (!Lasso || Lasso->State==ELassoState::Stored)
            {
                const auto* Herd=Mode->HerdManager.Get();
                if (!Herd || !Herd->FocusedHorse) { Objective=TEXT("NEXT  Look toward a wild horse and press Q"); }
                else if (!Herd->bTargetIsolated)
                {
                    Objective=FString::Printf(TEXT("NEXT  Drive the target away from the herd  %.0f%%"),Herd->IsolationProgress*100.f);
                }
                else { Objective=TEXT("NEXT  Hold RMB to ready the lasso"); }
            }
            else
            {
                switch (Lasso->State)
                {
                case ELassoState::Aiming: Objective=TEXT("NEXT  LMB to throw  |  release RMB to cancel"); break;
                case ELassoState::Thrown: Objective=TEXT("LOOP IN FLIGHT  Keep the target in line"); break;
                case ELassoState::Attached: Objective=TEXT("NEXT  Hold Space and keep tension in the green zone"); break;
                case ELassoState::Subdued: Objective=TEXT("NEXT  Press C to secure the horse"); break;
                case ELassoState::Recovering: Objective=TEXT("LASSO RECOVERING  Prepare another throw"); break;
                case ELassoState::Captured: Objective=TEXT("NEXT  Slow down and press E to dismount"); break;
                default: break;
                }
            }
            DrawRect(FLinearColor(0,0,0,.68f),18,MissionY-42,760,30);
            DrawText(Objective,FLinearColor(.55f,1.f,1.f),26,MissionY-36,nullptr,1.f);

            if (Mode->Trial.ElapsedSeconds<4.f)
            {
                const float Opacity=FMath::Clamp(4.f-Mode->Trial.ElapsedSeconds,0.f,1.f);
                DrawRect(FLinearColor(0,0,0,.78f*Opacity),Canvas->ClipX*.5f-270,130,540,92);
                DrawText(TEXT("ROUND START"),FLinearColor(1.f,.88f,.25f,Opacity),Canvas->ClipX*.5f-105,146,nullptr,1.65f);
                DrawText(TEXT("Capture, befriend, deliver and name one horse"),FLinearColor(1,1,1,Opacity),Canvas->ClipX*.5f-215,188,nullptr,1.05f);
            }
            if (Mode->Trial.RemainingSeconds<=10.f)
            {
                DrawText(TEXT("TIME RUNNING OUT"),FLinearColor(1.f,.15f,.08f,Pulse),Canvas->ClipX*.5f-125,240,nullptr,1.45f);
            }
        }
        else
        {
            const bool bSuccess=Mode->Trial.State==ESteppeTrialState::Success;
            DrawRect(FLinearColor(0,0,0,.78f),Canvas->ClipX*.5f-245,Canvas->ClipY*.5f-52,490,104);
            DrawText(bSuccess?TEXT("HORSE NAMED"):TEXT("TIME EXPIRED"),bSuccess?FLinearColor(.25f,1.f,.35f):FLinearColor(1.f,.25f,.15f),
                Canvas->ClipX*.5f-150,Canvas->ClipY*.5f-30,nullptr,1.8f);
            DrawText(FString::Printf(TEXT("Score %d  |  F2 replay"),Mode->Trial.Score),FLinearColor::White,
                Canvas->ClipX*.5f-105,Canvas->ClipY*.5f+12,nullptr,1.1f);
        }
        if (Mode->DeliveryZone && PlayerOwner)
        {
            FVector2D CampScreen;
            const FVector CampLocation=Mode->DeliveryZone->GetActorLocation()+FVector(0,0,240);
            if (PlayerOwner->ProjectWorldLocationToScreen(CampLocation,CampScreen))
            {
                DrawText(TEXT("CAMP / PEN"),FLinearColor(1.f,.75f,.2f),CampScreen.X-45,CampScreen.Y,nullptr,1.1f);
            }
        }
    }
    auto* Debug=GetWorld()->GetSubsystem<USteppeDebugSubsystem>();
    if (!Debug || (!Debug->IsHorseDebugEnabled() && !Debug->IsMovementDebugEnabled())) { return; }
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
            const int32 CapturedIndex=Lasso && Lasso->Target.IsValid()?Mode->WildHorses.IndexOfByKey(Lasso->Target.Get()):-1;
            const FString TargetLine=Lasso && Lasso->State==ELassoState::Captured && CapturedIndex!=INDEX_NONE
                ?FString::Printf(TEXT("CAPTURED: H%d | total secured %d"),CapturedIndex+1,Herd?Herd->CapturedCount:0)
                :Herd && Herd->FocusedHorse
                ?FString::Printf(TEXT("TARGET: H%d | separation %.1f m | isolate %.0f%%%s"),TargetIndex+1,
                    Herd->IsolationDistance/100.f,Herd->IsolationProgress*100.f,Herd->bTargetIsolated?TEXT(" | ISOLATED"):TEXT(""))
                :TEXT("TARGET: none | look toward a horse and press Q");
            DrawRect(FLinearColor(0,0,0,.65f),18,250,650,158);
            DrawText(FString::Printf(TEXT("WILD HERD: %d active | %d captured | alarm sources %d\n%s\nFOCUS: %s | awareness %.0f%% | neighbors %d\nDistance %.1f m | approach %.1f m/s | visible %s\nPath %s | cut the target away from the herd.\nLASSO: isolate, throw, hold Space, then C to capture."),
                Herd?Herd->Members.Num():Mode->WildHorses.Num(),Herd?Herd->CapturedCount:0,Herd?Herd->AlarmSourceCount:0,
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
                    const bool bCaptured=MemberState==EWildHorseState::Captured;
                    const FLinearColor Color=bCaptured?FLinearColor(.2f,1.f,.2f):bTarget?FLinearColor(0,1,1):MemberState==EWildHorseState::Fleeing?FLinearColor(1,.25f,.1f):
                        (MemberState==EWildHorseState::Alert?FLinearColor::Yellow:FLinearColor(.55f,.8f,1.f));
                    DrawText(FString::Printf(TEXT("%sH%d %s"),bCaptured?TEXT("CAPTURED "):(bTarget?TEXT("TARGET "):TEXT("")),Index+1,*UEnum::GetDisplayValueAsText(MemberState).ToString()),
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
