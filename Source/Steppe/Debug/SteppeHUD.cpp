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
#include "Character/Rider/RiderBalanceComponent.h"
#include "Character/Rider/RidingComponent.h"
#include "Feedback/SteppeFeedbackComponent.h"
#include "Capture/HorseTrustComponent.h"
#include "Engine/Canvas.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camp/SteppeDeliveryZone.h"
void ASteppeHUD::DrawHUD()
{
    Super::DrawHUD();
    DrawText(TEXT("STEPPE  |  F1 details  F2 restart"),FLinearColor(1.f,1.f,1.f,.7f),18,16,nullptr,.75f);
    auto* Rider=PlayerOwner?Cast<ASteppeRiderCharacter>(PlayerOwner->GetPawn()):nullptr;
    auto* Mode=GetWorld()->GetAuthGameMode<ASteppeGameMode>();
    auto* Lasso=Rider?Rider->Lasso.Get():nullptr;
    if (Rider && Rider->Riding)
    {
        if (const auto* Mount=Rider->Riding->GetHorse(); Mount && Mount->GetMesh()->DoesSocketExist(TEXT("Head")))
        {
            const FVector Head=Mount->GetMesh()->GetSocketLocation(TEXT("Head"));
            const FVector Side=Mount->GetActorRightVector()*9.f;
            const auto DrawRein=[this](const FVector& Hand,const FVector& Bit)
            {
                constexpr int32 Segments=10;
                FVector Previous=Hand;
                for (int32 Segment=1;Segment<=Segments;++Segment)
                {
                    const float Alpha=static_cast<float>(Segment)/Segments;
                    const FVector Current=FMath::Lerp(Hand,Bit,Alpha)-FVector::UpVector*(4.f*Alpha*(1.f-Alpha)*22.f);
                    DrawDebugLine(GetWorld(),Previous,Current,FColor(92,48,24),false,0,0,2.5f);
                    Previous=Current;
                }
            };
            DrawRein(Rider->GetReinHandLocation(true),Head-Side);
            const bool bRightHandOnLasso=Lasso && Lasso->State!=ELassoState::Stored && Lasso->State!=ELassoState::Recovering;
            if (!bRightHandOnLasso) { DrawRein(Rider->GetReinHandLocation(false),Head+Side); }
        }
    }
    if (Lasso)
    {
        const FString LassoText=FString::Printf(TEXT("LASSO %s | %s"),*UEnum::GetDisplayValueAsText(Lasso->State).ToString(),*Lasso->Feedback);
        const FLinearColor LassoColor=(Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Captured)?FLinearColor(.35f,1.f,.35f):
            (Lasso->State==ELassoState::Thrown?FLinearColor(1,.55f,.1f):FLinearColor(.5f,1.f,1.f));
        const float LassoX=FMath::Max(18.f,Canvas->ClipX-570.f);
        DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,44,558,25);
        DrawText(LassoText,LassoColor,LassoX,48,nullptr,.95f);
        if (Lasso->State==ELassoState::Aiming)
        {
            const float TargetDistance=Lasso->Target.IsValid() && Rider?FVector::Dist2D(Rider->GetActorLocation(),Lasso->Target->GetActorLocation()):BIG_NUMBER;
            const bool bInRange=TargetDistance<=Lasso->MaximumRange;
            const bool bStable=Lasso->bTargetIsolated && bInRange && Lasso->SwingStability>=.8f;
            const FLinearColor SwingColor=!Lasso->bTargetIsolated?FLinearColor(.65f,.65f,.65f):
                (!bInRange?FLinearColor(1.f,.2f,.12f):(bStable?FLinearColor(.3f,1.f,.3f):FLinearColor(1.f,.75f,.15f)));
            DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,73,558,43);
            DrawText(FString::Printf(TEXT("SWING %.0f%% | OPEN %.0f%% %s"),Lasso->SwingPhase*100.f,Lasso->SwingStability*100.f,
                bStable?TEXT("THROW"):(!Lasso->bTargetIsolated?TEXT("SEPARATE"):(!bInRange?TEXT("TOO FAR"):TEXT("WAIT")))),SwingColor,LassoX,78,nullptr,1.f);
            DrawRect(FLinearColor(.12f,.12f,.12f,1),LassoX,99,520,8);
            DrawRect(SwingColor,LassoX,99,520*Lasso->SwingStability,8);
        }
        if (Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Subdued || Lasso->State==ELassoState::Captured)
        {
            const bool bCaptureComplete=Lasso->State==ELassoState::Captured;
            const bool bGreen=bCaptureComplete || (Lasso->Tension>=Lasso->UsefulTensionMin && Lasso->Tension<=Lasso->UsefulTensionMax);
            const FLinearColor TensionColor=Lasso->bShockRisk?FLinearColor(1.f,.05f,.03f):(bGreen?FLinearColor(.3f,1.f,.3f):FLinearColor(1.f,.3f,.15f));
            DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,73,558,43);
            const float CalmDisplay=FMath::Max(Lasso->ControlProgress,Lasso->OnFootSurrenderProgress);
            const FString FightText=bCaptureComplete?TEXT("HORSE SURRENDERED | LEAD TO CAMP"):
                FString::Printf(TEXT("%s | TENSION %.0f%% %s | %s %.0f%%"),
                    *UEnum::GetDisplayValueAsText(Lasso->HitZone).ToString().ToUpper(),Lasso->Tension*100.f,
                    Lasso->bShockRisk?TEXT("SUDDEN JOLT"):(bGreen?TEXT("STEADY"):TEXT("ADJUST")),
                    Lasso->OnFootSurrenderProgress>0.f?TEXT("SURRENDER"):TEXT("CONTROL"),CalmDisplay*100.f);
            DrawText(FightText,TensionColor,LassoX,78,nullptr,1.f);
            DrawRect(FLinearColor(.12f,.12f,.12f,1),LassoX,99,520,8);
            DrawRect(FLinearColor(.3f,.8f,1.f,1),LassoX,99,520*Lasso->ControlProgress,8);
        }
        if (Rider->Balance && (Lasso->State==ELassoState::Attached || Rider->Balance->State!=ERiderBalanceState::Stable))
        {
            const float NormalizedBalance=FMath::Clamp(Rider->Balance->Balance/FMath::Max(.01f,Rider->Balance->FallThreshold),0.f,1.f);
            const bool bDanger=Rider->Balance->State==ERiderBalanceState::Warning || Rider->Balance->State==ERiderBalanceState::Falling || Rider->Balance->State==ERiderBalanceState::Dragged || Rider->Balance->State==ERiderBalanceState::Pulled;
            const FLinearColor BalanceColor=bDanger?FLinearColor(1.f,.28f,.12f):FLinearColor(.4f,1.f,.55f);
            DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,121,558,43);
            DrawText(FString::Printf(TEXT("BALANCE %.0f%% | SIDE %.0f%% | %s"),NormalizedBalance*100.f,Rider->Balance->LateralPull*100.f,
                *UEnum::GetDisplayValueAsText(Rider->Balance->State).ToString().ToUpper()),BalanceColor,LassoX,126,nullptr,1.f);
            DrawRect(FLinearColor(.12f,.12f,.12f,1),LassoX,147,520,8);
            DrawRect(BalanceColor,LassoX,147,520*NormalizedBalance,8);
        }
        if (Rider->Feedback && GetWorld()->GetSubsystem<USteppeDebugSubsystem>()->IsHorseDebugEnabled())
        {
            const auto EventText=UEnum::GetDisplayValueAsText(Rider->Feedback->LastEvent).ToString().ToUpper();
            DrawRect(FLinearColor(0,0,0,.6f),LassoX-6,169,558,43);
            DrawText(FString::Printf(TEXT("%s | HOOF %d | WIND %.0f%% | BREATH %.0f%% | %s"),
                *UEnum::GetDisplayValueAsText(Rider->Feedback->GroundSurface).ToString().ToUpper(),Rider->Feedback->HoofbeatCount,
                Rider->Feedback->WindIntensity*100.f,Rider->Feedback->BreathIntensity*100.f,*EventText),
                FLinearColor(.65f,.9f,1.f),LassoX,174,nullptr,.9f);
            DrawRect(FLinearColor(.12f,.12f,.12f,1),LassoX,197,520,7);
            DrawRect(FLinearColor(1.f,.48f,.12f,1),LassoX,197,520*Rider->Feedback->RopeStress,7);
        }
        if (Lasso->State==ELassoState::Aiming)
        {
            const float TargetDistance=Lasso->Target.IsValid() && Rider?FVector::Dist2D(Rider->GetActorLocation(),Lasso->Target->GetActorLocation()):BIG_NUMBER;
            const bool bInRange=TargetDistance<=Lasso->MaximumRange;
            const bool bThrowReady=Lasso->bTargetIsolated && bInRange && Lasso->SwingStability>=.8f;
            const FLinearColor ReticleColor=bThrowReady?FLinearColor(.3f,1.f,.3f):FLinearColor::White;
            DrawText(TEXT("+"),ReticleColor,Canvas->ClipX*.5f-5,Canvas->ClipY*.5f-12,nullptr,1.5f);

            // A readable screen-space loop makes the lasso visible before it is thrown.
            const FVector2D LoopCenter(Canvas->ClipX*.5f,Canvas->ClipY*.5f);
            const float Pulse=.88f+.12f*FMath::Sin(Lasso->SwingPhase*2.f*PI);
            const float RadiusX=(48.f+Lasso->SwingStability*22.f)*Pulse;
            const float RadiusY=RadiusX*.62f;
            const FLinearColor LoopColor=!Lasso->bTargetIsolated?FLinearColor(.62f,.62f,.62f):
                (!bInRange?FLinearColor(1.f,.15f,.08f):(bThrowReady?FLinearColor(.2f,1.f,.25f):FLinearColor(1.f,.72f,.12f)));
            constexpr int32 LoopSegments=40;
            for (int32 Segment=0; Segment<LoopSegments; ++Segment)
            {
                const float A0=2.f*PI*Segment/LoopSegments;
                const float A1=2.f*PI*(Segment+1)/LoopSegments;
                const FVector2D P0=LoopCenter+FVector2D(FMath::Cos(A0)*RadiusX,FMath::Sin(A0)*RadiusY);
                const FVector2D P1=LoopCenter+FVector2D(FMath::Cos(A1)*RadiusX,FMath::Sin(A1)*RadiusY);
                DrawLine(P0.X,P0.Y,P1.X,P1.Y,LoopColor,bThrowReady?6.f:4.f);
            }
        }
        if (Lasso->State==ELassoState::Thrown || Lasso->State==ELassoState::Attached || Lasso->State==ELassoState::Subdued || Lasso->State==ELassoState::Captured)
        {
            const float Stress=FMath::Clamp(Lasso->Tension/1.2f,0.f,1.f);
            const bool bUseful=Lasso->Tension>=Lasso->UsefulTensionMin && Lasso->Tension<=Lasso->UsefulTensionMax;
            const FColor RopeColor=Lasso->Tension>1.f?FColor::Red:(Lasso->Tension>.85f?FColor(255,96,20):(bUseful?FColor::Green:FColor::Yellow));
            const float RopeSag=Lasso->State==ELassoState::Thrown?55.f:FMath::Lerp(95.f,5.f,Stress);
            const float RopeThickness=4.f+Stress*8.f;
            const auto DrawRopeSpan=[this,&RopeColor,RopeThickness](const FVector& Start, const FVector& End, float Sag)
            {
                constexpr int32 RopeSegments=18;
                FVector Previous=Start;
                for (int32 Segment=1; Segment<=RopeSegments; ++Segment)
                {
                    const float Alpha=static_cast<float>(Segment)/RopeSegments;
                    const FVector Current=FMath::Lerp(Start,End,Alpha)-FVector::UpVector*(4.f*Alpha*(1.f-Alpha)*Sag);
                    DrawDebugLine(GetWorld(),Previous,Current,RopeColor,false,0,0,RopeThickness);
                    Previous=Current;
                }
            };
            if (Lasso->bRopeWrapped)
            {
                DrawRopeSpan(Lasso->VisualRopeStart,Lasso->RopeBendPoint,RopeSag*.5f);
                DrawRopeSpan(Lasso->RopeBendPoint,Lasso->LoopLocation,RopeSag*.5f);
                DrawDebugSphere(GetWorld(),Lasso->RopeBendPoint,18.f,10,FColor(255,128,20),false,0,0,4.f);
            }
            else
            {
                DrawRopeSpan(Lasso->VisualRopeStart,Lasso->LoopLocation,RopeSag);
            }
            if (Lasso->State!=ELassoState::Captured)
            {
                constexpr int32 PhysicalLoopSegments=32;
                for (int32 Segment=0; Segment<PhysicalLoopSegments; ++Segment)
                {
                    const float A0=2.f*PI*Segment/PhysicalLoopSegments;
                    const float A1=2.f*PI*(Segment+1)/PhysicalLoopSegments;
                    const FVector P0=Lasso->LoopLocation+Lasso->LoopAxisX*(FMath::Cos(A0)*Lasso->LoopRadius)
                        +Lasso->LoopAxisY*(FMath::Sin(A0)*Lasso->LoopRadius);
                    const FVector P1=Lasso->LoopLocation+Lasso->LoopAxisX*(FMath::Cos(A1)*Lasso->LoopRadius)
                        +Lasso->LoopAxisY*(FMath::Sin(A1)*Lasso->LoopRadius);
                    DrawDebugLine(GetWorld(),P0,P1,RopeColor,false,0,0,6.f);
                }
            }
        }
    }
    if (Rider && Rider->Feedback && Rider->Feedback->DustPulse>0.f && Rider->Riding)
    {
        if (const auto* Horse=Rider->Riding->GetHorse())
        {
            const FVector Rear=Horse->GetActorLocation()-Horse->GetActorForwardVector()*185.f+FVector(0,0,-55.f);
            const FVector Side=Horse->GetActorRightVector()*82.f;
            const float Radius=18.f+Rider->Feedback->DustPulse*48.f;
            const FColor Dust(166,112,62);
            DrawDebugSphere(GetWorld(),Rear+Side,Radius,8,Dust,false,0,0,2.5f);
            DrawDebugSphere(GetWorld(),Rear-Side,Radius*.8f,8,Dust,false,0,0,2.f);
            DrawDebugSphere(GetWorld(),Rear-Horse->GetActorForwardVector()*65.f,Radius*.65f,8,FColor(194,142,84),false,0,0,1.5f);
        }
    }
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
                        Objective=TEXT("WAIT  The led horse is catching up - stay near it");
                    }
                    else { Objective=FString::Printf(TEXT("NEXT  %s to CAMP / PEN  |  %.1f m"),
                        Rider && Rider->Riding && Rider->Riding->IsMounted()?TEXT("Ride slowly with the horse"):TEXT("Walk with the horse"),CampDistance); }
                }
                else if (PendingTrust->State==EPostCaptureState::FirstContact)
                {
                    Objective=TEXT("NEXT  Press E to take the lead rope");
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
                case ELassoState::Aiming:
                    if (!Lasso->bTargetIsolated) { Objective=TEXT("AIM ACTIVE  Separate the target from the herd"); }
                    else if (Lasso->Target.IsValid() && Rider && FVector::Dist2D(Rider->GetActorLocation(),Lasso->Target->GetActorLocation())>Lasso->MaximumRange)
                    { Objective=TEXT("TARGET TOO FAR  Close the distance before throwing"); }
                    else { Objective=Lasso->SwingStability>=.8f?TEXT("THROW  Stable loop - press LMB now"):TEXT("WAIT  Build the swing and watch OPEN"); }
                    break;
                case ELassoState::Thrown: Objective=TEXT("LOOP IN FLIGHT  Keep the target in line"); break;
                case ELassoState::Attached: Objective=TEXT("NEXT  Hold Space and keep tension in the green zone"); break;
                case ELassoState::Subdued: Objective=TEXT("NEXT  Press C to secure the horse"); break;
                case ELassoState::Recovering: Objective=TEXT("LASSO RECOVERING  Prepare another throw"); break;
                case ELassoState::Captured: Objective=TEXT("NEXT  Slow down and press E to dismount"); break;
                default: break;
                }
            }

            // Keep the current action near the player's focal area instead of relying on the small top help line.
            FString ActionTitle;
            FString ActionDetail;
            FLinearColor ActionColor(1.f,.82f,.2f);
            const auto* Herd=Mode->HerdManager.Get();
            ASteppeWildHorseCharacter* FocusHorse=Herd?Herd->FocusedHorse.Get():nullptr;
            ASteppeWildHorseCharacter* PreviewHorse=nullptr;
            if (Herd && Rider && !FocusHorse && PlayerOwner)
            {
                PreviewHorse=Herd->FindFocusHorse(Rider->GetActorLocation(),PlayerOwner->GetControlRotation().Vector());
            }
            if ((!Lasso || Lasso->State==ELassoState::Stored) && !FocusHorse)
            {
                ActionTitle=PreviewHorse?TEXT("[ Q ]  SELECT THIS HORSE"):TEXT("FIND A WILD HORSE");
                ActionDetail=PreviewHorse?TEXT("Press Q now, then chase the marked horse away from the herd"):TEXT("Look at a horse and place it near the screen center");
            }
            else if ((!Lasso || Lasso->State==ELassoState::Stored) && FocusHorse && Herd && !Herd->bTargetIsolated)
            {
                ActionTitle=TEXT("TARGET MARKED");
                ActionDetail=FString::Printf(TEXT("Chase this horse away from the herd  %.0f%%"),Herd->IsolationProgress*100.f);
                ActionColor=FLinearColor(.2f,1.f,1.f);
            }
            else if ((!Lasso || Lasso->State==ELassoState::Stored) && FocusHorse && Herd && Herd->bTargetIsolated)
            {
                const float Distance=Rider?FVector::Dist2D(Rider->GetActorLocation(),FocusHorse->GetActorLocation()):0.f;
                if (Lasso && Distance>Lasso->MaximumRange)
                {
                    ActionTitle=TEXT("TARGET TOO FAR  |  CLOSE THE DISTANCE");
                    ActionDetail=FString::Printf(TEXT("Distance %.0f m  |  Lasso range %.0f m"),Distance/100.f,Lasso->MaximumRange/100.f);
                    ActionColor=FLinearColor(1.f,.25f,.12f);
                }
                else
                {
                    ActionTitle=TEXT("TARGET ISOLATED  |  HOLD [ RMB ]");
                    ActionDetail=TEXT("Keep Right Mouse held to swing the lasso");
                    ActionColor=FLinearColor(.3f,1.f,.35f);
                }
            }
            else if (Lasso && Lasso->State==ELassoState::Aiming)
            {
                const float Distance=Lasso->Target.IsValid() && Rider?FVector::Dist2D(Rider->GetActorLocation(),Lasso->Target->GetActorLocation()):BIG_NUMBER;
                const bool bInRange=Distance<=Lasso->MaximumRange;
                const bool bReady=Lasso->bTargetIsolated && bInRange && Lasso->SwingStability>=.8f;
                if (!Lasso->bTargetIsolated)
                {
                    ActionTitle=TEXT("AIM ACTIVE  |  SEPARATE THE TARGET");
                    ActionDetail=FString::Printf(TEXT("Move it %.0f m from the herd to unlock the throw  %.0f%%"),
                        Herd?Herd->IsolationDistanceRequired/100.f:0.f,Herd?Herd->IsolationProgress*100.f:0.f);
                    ActionColor=FLinearColor(.72f,.72f,.72f);
                }
                else if (!bInRange)
                {
                    ActionTitle=TEXT("RED LOOP  |  TARGET TOO FAR");
                    ActionDetail=FString::Printf(TEXT("Close to %.0f m or less before throwing  |  now %.0f m"),Lasso->MaximumRange/100.f,Distance/100.f);
                    ActionColor=FLinearColor(1.f,.2f,.1f);
                }
                else
                {
                    ActionTitle=bReady?TEXT("GREEN LOOP  |  PRESS [ LMB ] NOW"):TEXT("KEEP [ RMB ] HELD");
                    ActionDetail=bReady?TEXT("Throw while the loop is green and the horse is inside it"):TEXT("Wait for the visible loop to open and turn green");
                    ActionColor=bReady?FLinearColor(.2f,1.f,.25f):FLinearColor(1.f,.72f,.12f);
                }
            }
            else if (Lasso && Lasso->State==ELassoState::Thrown)
            {
                ActionTitle=TEXT("LOOP IN FLIGHT");
                ActionDetail=TEXT("Keep the horse inside the loop");
                ActionColor=FLinearColor(1.f,.65f,.12f);
            }
            else if (Lasso && Lasso->State==ELassoState::Attached)
            {
                if (Rider && Rider->Balance && Rider->Balance->State==ERiderBalanceState::Dragged)
                {
                    ActionTitle=TEXT("YOU ARE BEING DRAGGED");
                    ActionDetail=FString::Printf(TEXT("Hold Space to keep controlling the horse  |  RMB releases  |  stand in %.1f s"),Rider->Balance->DraggedRemaining);
                    ActionColor=FLinearColor(1.f,.08f,.03f);
                }
                else if (Rider && Rider->Balance && Rider->Balance->State==ERiderBalanceState::Pulled)
                {
                    const float Distance=Lasso->Target.IsValid()?FVector::Dist2D(Rider->GetActorLocation(),Lasso->Target->GetActorLocation()):BIG_NUMBER;
                    if (Distance<=Lasso->OnFootSurrenderDistance && Lasso->bBracing)
                    {
                        ActionTitle=FString::Printf(TEXT("STAY CLOSE  |  HORSE SURRENDERING %.0f%%"),Lasso->OnFootSurrenderProgress*100.f);
                        ActionDetail=TEXT("Keep Space held until the horse accepts the lead rope");
                        ActionColor=FLinearColor(.25f,1.f,.3f);
                    }
                    else if (Distance<=Lasso->OnFootSurrenderDistance)
                    {
                        ActionTitle=TEXT("CLOSE ENOUGH  |  HOLD [ SPACE ]");
                        ActionDetail=TEXT("Match the horse's movement and hold the rope steady to make it surrender");
                        ActionColor=FLinearColor(1.f,.72f,.12f);
                    }
                    else
                    {
                        ActionTitle=TEXT("HORSE IS PULLING YOU  |  GET WITHIN 3 M");
                        ActionDetail=TEXT("Run with it, move close, then hold Space to make it surrender");
                        ActionColor=FLinearColor(1.f,.32f,.06f);
                    }
                }
                else if (Lasso->bShockRisk)
                {
                    ActionTitle=TEXT("SUDDEN JOLT  |  KEEP MOVING");
                    ActionDetail=TEXT("An abrupt stop can pull you off the saddle or snap the rope");
                    ActionColor=FLinearColor(1.f,.04f,.02f);
                }
                else if (Rider && Rider->Balance && Rider->Balance->State==ERiderBalanceState::Warning)
                {
                    ActionTitle=TEXT("HORSE IS PULLING YOU OFF BALANCE");
                    ActionDetail=TEXT("Turn toward the rope or slow down  |  RMB releases the rope");
                    ActionColor=FLinearColor(1.f,.18f,.05f);
                }
                else if (Lasso->Tension>Lasso->UsefulTensionMax)
                {
                    ActionTitle=TEXT("HORSE PULLING HARD  |  MOVE WITH IT");
                    ActionDetail=TEXT("Sustained pull drags the rider; avoid a sudden high-speed stop");
                    ActionColor=FLinearColor(1.f,.2f,.08f);
                }
                else if (!Lasso->bBracing)
                {
                    ActionTitle=TEXT("[ SPACE ]  HOLD TO CONTROL THE HORSE");
                    ActionDetail=TEXT("Keep rope tension inside the green zone");
                    ActionColor=FLinearColor(1.f,.75f,.12f);
                }
                else if (Lasso->Tension<Lasso->UsefulTensionMin)
                {
                    ActionTitle=TEXT("ROPE SLACK  |  CREATE SOME DISTANCE");
                    ActionDetail=TEXT("Keep holding Space and move until tension enters green");
                    ActionColor=FLinearColor(1.f,.75f,.12f);
                }
                else
                {
                    ActionTitle=FString::Printf(TEXT("KEEP STEADY  |  HORSE CALMING %.0f%%"),Lasso->ControlProgress*100.f);
                    ActionDetail=TEXT("Hold Space and keep the tension marker inside green");
                    ActionColor=FLinearColor(.25f,1.f,.3f);
                }
            }
            else if (Lasso && Lasso->State==ELassoState::Subdued)
            {
                ActionTitle=TEXT("HORSE CALM  |  PRESS [ C ] TO SECURE");
                ActionDetail=TEXT("The struggle is over; complete the capture");
                ActionColor=FLinearColor(.25f,1.f,.3f);
            }
            else if (Lasso && Lasso->State==ELassoState::Recovering)
            {
                const bool bBroke=Lasso->Feedback.Contains(TEXT("broke"),ESearchCase::IgnoreCase);
                ActionTitle=bBroke?TEXT("ROPE BROKE  |  HORSE ESCAPED"):TEXT("LASSO MISSED  |  RECOVERING");
                ActionDetail=TEXT("Follow the marked horse and prepare another throw");
                ActionColor=bBroke?FLinearColor(1.f,.1f,.04f):FLinearColor(1.f,.62f,.12f);
            }
            else if (Lasso && Lasso->State==ELassoState::Captured && Lasso->Target.IsValid()
                && Lasso->Target->Trust && Lasso->Target->Trust->State==EPostCaptureState::Leading)
            {
                ActionTitle=TEXT("HORSE SURRENDERED  |  LEAD IT TO CAMP");
                ActionDetail=TEXT("Walk toward CAMP / PEN; the horse will now follow you");
                ActionColor=FLinearColor(.25f,1.f,.3f);
            }
            if (Lasso && Lasso->State==ELassoState::Attached && Lasso->bRopeWrapped && !ActionDetail.IsEmpty())
            {
                ActionDetail+=TEXT("  |  OBSTACLE BEND IS SLOWING THE HORSE");
            }
            if (!ActionTitle.IsEmpty() && (FocusHorse || PreviewHorse || (Lasso && Lasso->State!=ELassoState::Stored)))
            {
                const float CardW=FMath::Min(480.f,Canvas->ClipX-36.f);
                const float CardX=Canvas->ClipX-CardW-18.f;
                const bool bRopeFight=Lasso && Lasso->State==ELassoState::Attached;
                const float CardH=bRopeFight?100.f:42.f;
                const float CardY=Canvas->ClipY-CardH-90.f;
                DrawRect(FLinearColor(0,0,0,.62f),CardX,CardY,CardW,CardH);
                DrawRect(ActionColor,CardX,CardY,CardW,3.f);
                DrawText(ActionTitle.Left(49),ActionColor,CardX+12.f,CardY+9.f,nullptr,1.f);
                if (bRopeFight)
                {
                    const float BarX=CardX+112.f;
                    const float BarW=CardW-126.f;
                    const float TensionY=CardY+48.f;
                    const float ControlY=CardY+73.f;
                    DrawText(TEXT("TENSION"),FLinearColor::White,CardX+12.f,TensionY-4.f,nullptr,.75f);
                    DrawRect(FLinearColor(.13f,.13f,.13f,1.f),BarX,TensionY,BarW,12.f);
                    DrawRect(FLinearColor(.12f,.6f,.16f,1.f),BarX+BarW*(Lasso->UsefulTensionMin/1.2f),TensionY,
                        BarW*((Lasso->UsefulTensionMax-Lasso->UsefulTensionMin)/1.2f),12.f);
                    const float MarkerX=BarX+BarW*FMath::Clamp(Lasso->Tension/1.2f,0.f,1.f);
                    DrawRect(FLinearColor::White,MarkerX-3.f,TensionY-4.f,6.f,20.f);
                    DrawText(Lasso->OnFootSurrenderProgress>0.f?TEXT("SURRENDER"):TEXT("CALMING"),FLinearColor::White,CardX+12.f,ControlY-4.f,nullptr,.75f);
                    DrawRect(FLinearColor(.13f,.13f,.13f,1.f),BarX,ControlY,BarW,12.f);
                    DrawRect(FLinearColor(.2f,.65f,1.f,1.f),BarX,ControlY,BarW*FMath::Max(Lasso->ControlProgress,Lasso->OnFootSurrenderProgress),12.f);
                }
            }

            // Brackets show exactly which horse Q will select, and remain on the chosen target afterward.
            ASteppeWildHorseCharacter* MarkedHorse=FocusHorse?FocusHorse:PreviewHorse;
            if (MarkedHorse && PlayerOwner)
            {
                FVector2D HorseScreen;
                if (PlayerOwner->ProjectWorldLocationToScreen(MarkedHorse->GetActorLocation()+FVector(0,0,105),HorseScreen))
                {
                    const bool bSelected=FocusHorse!=nullptr;
                    const float Distance=Rider?FVector::Dist2D(Rider->GetActorLocation(),MarkedHorse->GetActorLocation()):0.f;
                    const bool bTooFar=bSelected && Lasso && Distance>Lasso->MaximumRange;
                    const FLinearColor MarkerColor=bTooFar?FLinearColor(1.f,.2f,.08f):(bSelected?FLinearColor(.2f,1.f,1.f):FLinearColor(1.f,.82f,.2f));
                    const float HalfW=48.f, HalfH=58.f, Corner=18.f, Thick=bSelected?4.f:6.f;
                    DrawLine(HorseScreen.X-HalfW,HorseScreen.Y-HalfH,HorseScreen.X-HalfW+Corner,HorseScreen.Y-HalfH,MarkerColor,Thick);
                    DrawLine(HorseScreen.X-HalfW,HorseScreen.Y-HalfH,HorseScreen.X-HalfW,HorseScreen.Y-HalfH+Corner,MarkerColor,Thick);
                    DrawLine(HorseScreen.X+HalfW,HorseScreen.Y-HalfH,HorseScreen.X+HalfW-Corner,HorseScreen.Y-HalfH,MarkerColor,Thick);
                    DrawLine(HorseScreen.X+HalfW,HorseScreen.Y-HalfH,HorseScreen.X+HalfW,HorseScreen.Y-HalfH+Corner,MarkerColor,Thick);
                    DrawLine(HorseScreen.X-HalfW,HorseScreen.Y+HalfH,HorseScreen.X-HalfW+Corner,HorseScreen.Y+HalfH,MarkerColor,Thick);
                    DrawLine(HorseScreen.X-HalfW,HorseScreen.Y+HalfH,HorseScreen.X-HalfW,HorseScreen.Y+HalfH-Corner,MarkerColor,Thick);
                    DrawLine(HorseScreen.X+HalfW,HorseScreen.Y+HalfH,HorseScreen.X+HalfW-Corner,HorseScreen.Y+HalfH,MarkerColor,Thick);
                    DrawLine(HorseScreen.X+HalfW,HorseScreen.Y+HalfH,HorseScreen.X+HalfW,HorseScreen.Y+HalfH-Corner,MarkerColor,Thick);
                    const FString MarkerText=bSelected?FString::Printf(TEXT("%s  %.0f m"),bTooFar?TEXT("TOO FAR"):TEXT("TARGET"),Distance/100.f):TEXT("Q  SELECT");
                    DrawText(MarkerText,MarkerColor,HorseScreen.X-52.f,HorseScreen.Y-HalfH-25.f,nullptr,1.05f);
                }
            }
            if (Rider && Rider->Balance)
            {
                if (Rider->Balance->State==ERiderBalanceState::Dragged) { Objective=TEXT("DRAGGED  Hold Space to control; RMB releases"); }
                else if (Rider->Balance->State==ERiderBalanceState::Pulled) { Objective=TEXT("PULLED ON FOOT  Run with the horse and hold Space"); }
                else if (Rider->Balance->State==ERiderBalanceState::Falling) { Objective=TEXT("FALL  Release the rope and recover"); }
                else if (Rider->Balance->State==ERiderBalanceState::Warning) { Objective=TEXT("BALANCE WARNING  Turn toward the rope or slow down"); }
            }
            DrawRect(FLinearColor(0,0,0,.68f),18,MissionY-42,760,30);
            DrawText(Objective,FLinearColor(.55f,1.f,1.f),26,MissionY-36,nullptr,1.f);

            if (Mode->Trial.ElapsedSeconds<4.f && (!Herd || !Herd->FocusedHorse))
            {
                const float Opacity=FMath::Clamp(4.f-Mode->Trial.ElapsedSeconds,0.f,1.f);
                DrawRect(FLinearColor(0,0,0,.78f*Opacity),Canvas->ClipX*.5f-270,130,540,92);
                DrawText(TEXT("ROUND START"),FLinearColor(1.f,.88f,.25f,Opacity),Canvas->ClipX*.5f-105,146,nullptr,1.65f);
                DrawText(TEXT("LOOK AT A HORSE  >  PRESS Q TO MARK IT"),FLinearColor(1,1,1,Opacity),Canvas->ClipX*.5f-220,188,nullptr,1.05f);
            }
            if (Mode->Trial.RemainingSeconds<=10.f)
            {
                DrawText(TEXT("TIME RUNNING OUT"),FLinearColor(1.f,.15f,.08f,Pulse),Canvas->ClipX*.5f-125,240,nullptr,1.45f);
            }
        }
        else
        {
            const bool bSuccess=Mode->Trial.State==ESteppeTrialState::Success;
            const float PanelX=Canvas->ClipX*.5f-280.f;
            const float PanelY=Canvas->ClipY*.5f-82.f;
            const FLinearColor Accent=bSuccess?FLinearColor(.25f,1.f,.35f):FLinearColor(1.f,.25f,.15f);
            DrawRect(FLinearColor(0,0,0,.84f),PanelX,PanelY,560,164);
            DrawRect(Accent,PanelX,PanelY,560,5);
            DrawText(bSuccess?TEXT("ROUND COMPLETE"):TEXT("ROUND FAILED"),Accent,PanelX+190,PanelY+18,nullptr,1.05f);
            DrawText(bSuccess?TEXT("HORSE NAMED"):TEXT("TIME EXPIRED"),FLinearColor::White,PanelX+145,PanelY+48,nullptr,1.9f);
            DrawText(FString::Printf(TEXT("SECURED %d   CONTACT %d   DELIVERED %d   NAMED %d"),Mode->Trial.Captured,
                Mode->Trial.FirstContacts,Mode->Trial.Delivered,Mode->Trial.Named),FLinearColor(.7f,.9f,.82f),PanelX+82,PanelY+96,nullptr,1.f);
            DrawText(FString::Printf(TEXT("SCORE %d   |   F2 REPLAY ROUND"),Mode->Trial.Score),FLinearColor(1.f,.82f,.25f),
                PanelX+145,PanelY+128,nullptr,1.05f);
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
                    DrawText(FString::Printf(TEXT("%sH%d [%s] %s"),bCaptured?TEXT("CAPTURED "):(bTarget?TEXT("TARGET "):TEXT("")),Index+1,
                        *Member->ArchetypeLabel,*UEnum::GetDisplayValueAsText(MemberState).ToString()),
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
