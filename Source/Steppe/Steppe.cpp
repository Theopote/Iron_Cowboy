#include "Steppe.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "AnimationGraph.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#endif
DEFINE_LOG_CATEGORY(LogSteppe);
DEFINE_LOG_CATEGORY(LogSteppeHorse);
DEFINE_LOG_CATEGORY(LogSteppeRiding);
class FSteppeModule final : public FDefaultGameModuleImpl
{
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
        if (FParse::Param(FCommandLine::Get(),TEXT("SteppeBuildRiderAnimGraph")))
        {
            UAnimBlueprint* Blueprint=LoadObject<UAnimBlueprint>(nullptr,
                TEXT("/Game/Steppe/Presentation/ABP_Rider.ABP_Rider"));
            UAnimSequence* Idle=LoadObject<UAnimSequence>(nullptr,
                TEXT("/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle"));
            if (!Blueprint || !Idle) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimBlueprint or idle animation missing")); return; }
            TArray<UEdGraph*> Graphs;
            Blueprint->GetAllGraphs(Graphs);
            UAnimationGraph* AnimGraph=nullptr;
            for (UEdGraph* Graph : Graphs)
            {
                if (Graph->GetName()==TEXT("AnimGraph")) { AnimGraph=Cast<UAnimationGraph>(Graph); break; }
            }
            if (!AnimGraph) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimGraph missing")); return; }
            UAnimGraphNode_Root* Root=nullptr;
            for (UEdGraphNode* Node : AnimGraph->Nodes)
            {
                if (auto* Candidate=Cast<UAnimGraphNode_Root>(Node)) { Root=Candidate; break; }
            }
            if (!Root) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimGraph root missing")); return; }
            bool bAlreadyBuilt=false;
            for (UEdGraphNode* Node : AnimGraph->Nodes) { bAlreadyBuilt|=Node->IsA<UAnimGraphNode_Slot>(); }
            if (!bAlreadyBuilt)
            {
                FGraphNodeCreator<UAnimGraphNode_SequencePlayer> IdleCreator(*AnimGraph);
                UAnimGraphNode_SequencePlayer* IdleNode=IdleCreator.CreateNode();
                IdleNode->NodePosX=-500; IdleNode->NodePosY=0;
                IdleCreator.Finalize();
                IdleNode->SetAnimationAsset(Idle);
                IdleNode->ReconstructNode();
                FGraphNodeCreator<UAnimGraphNode_Slot> SlotCreator(*AnimGraph);
                UAnimGraphNode_Slot* SlotNode=SlotCreator.CreateNode();
                SlotNode->NodePosX=-250; SlotNode->NodePosY=0;
                SlotNode->Node.SlotName=TEXT("DefaultSlot");
                SlotCreator.Finalize();
                auto FindPosePin=[](UEdGraphNode* Node,EEdGraphPinDirection Direction)
                {
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction==Direction && Pin->PinType.PinCategory==TEXT("struct")) { return Pin; }
                    }
                    return static_cast<UEdGraphPin*>(nullptr);
                };
                const UEdGraphSchema* Schema=AnimGraph->GetSchema();
                const bool bSource=Schema->TryCreateConnection(FindPosePin(IdleNode,EGPD_Output),FindPosePin(SlotNode,EGPD_Input));
                const bool bResult=Schema->TryCreateConnection(FindPosePin(SlotNode,EGPD_Output),FindPosePin(Root,EGPD_Input));
                UE_LOG(LogSteppe,Display,TEXT("Rider AnimGraph links idle-to-slot=%d slot-to-root=%d"),bSource,bResult);
                if (!bSource || !bResult) { return; }
                AnimGraph->NotifyGraphChanged();
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                FKismetEditorUtilities::CompileBlueprint(Blueprint);
                Blueprint->MarkPackageDirty();
            }
            UE_LOG(LogSteppe,Display,TEXT("Rider AnimGraph ready nodes=%d"),AnimGraph->Nodes.Num());
            return;
        }
        if (FParse::Param(FCommandLine::Get(),TEXT("SteppeBuildHorseAnimGraph")))
        {
            UAnimBlueprint* Blueprint=LoadObject<UAnimBlueprint>(nullptr,
                TEXT("/Game/Steppe/Presentation/ABP_Horse.ABP_Horse"));
            UAnimSequence* Idle=LoadObject<UAnimSequence>(nullptr,
                TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/HorseIdle.HorseIdle"));
            if (!Blueprint || !Idle) { UE_LOG(LogSteppe,Error,TEXT("Horse AnimBlueprint or idle animation missing")); return; }
            TArray<UEdGraph*> Graphs;
            Blueprint->GetAllGraphs(Graphs);
            UAnimationGraph* AnimGraph=nullptr;
            for (UEdGraph* Graph : Graphs)
            {
                if (Graph->GetName()==TEXT("AnimGraph")) { AnimGraph=Cast<UAnimationGraph>(Graph); break; }
            }
            if (!AnimGraph) { UE_LOG(LogSteppe,Error,TEXT("Horse AnimGraph missing")); return; }
            UAnimGraphNode_Root* Root=nullptr;
            for (UEdGraphNode* Node : AnimGraph->Nodes)
            {
                if (auto* Candidate=Cast<UAnimGraphNode_Root>(Node)) { Root=Candidate; break; }
            }
            if (!Root) { UE_LOG(LogSteppe,Error,TEXT("Horse AnimGraph root missing")); return; }
            bool bAlreadyBuilt=false;
            for (UEdGraphNode* Node : AnimGraph->Nodes) { bAlreadyBuilt|=Node->IsA<UAnimGraphNode_Slot>(); }
            if (!bAlreadyBuilt)
            {
                FGraphNodeCreator<UAnimGraphNode_SequencePlayer> IdleCreator(*AnimGraph);
                UAnimGraphNode_SequencePlayer* IdleNode=IdleCreator.CreateNode();
                IdleNode->NodePosX=-500; IdleNode->NodePosY=0;
                IdleCreator.Finalize();
                IdleNode->SetAnimationAsset(Idle);
                IdleNode->ReconstructNode();
                FGraphNodeCreator<UAnimGraphNode_Slot> SlotCreator(*AnimGraph);
                UAnimGraphNode_Slot* SlotNode=SlotCreator.CreateNode();
                SlotNode->NodePosX=-250; SlotNode->NodePosY=0;
                SlotNode->Node.SlotName=TEXT("DefaultSlot");
                SlotCreator.Finalize();
                auto FindPosePin=[](UEdGraphNode* Node,EEdGraphPinDirection Direction)
                {
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction==Direction && Pin->PinType.PinCategory==TEXT("struct")) { return Pin; }
                    }
                    return static_cast<UEdGraphPin*>(nullptr);
                };
                const UEdGraphSchema* Schema=AnimGraph->GetSchema();
                const bool bSource=Schema->TryCreateConnection(FindPosePin(IdleNode,EGPD_Output),FindPosePin(SlotNode,EGPD_Input));
                const bool bResult=Schema->TryCreateConnection(FindPosePin(SlotNode,EGPD_Output),FindPosePin(Root,EGPD_Input));
                UE_LOG(LogSteppe,Display,TEXT("Horse AnimGraph links idle-to-slot=%d slot-to-root=%d"),bSource,bResult);
                if (!bSource || !bResult) { return; }
                AnimGraph->NotifyGraphChanged();
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                FKismetEditorUtilities::CompileBlueprint(Blueprint);
                Blueprint->MarkPackageDirty();
            }
            UE_LOG(LogSteppe,Display,TEXT("Horse AnimGraph ready nodes=%d"),AnimGraph->Nodes.Num());
            return;
        }
        if (!FParse::Param(FCommandLine::Get(),TEXT("SteppeFixSockets"))) { return; }
        const auto Fix=[](const TCHAR* Path, const TCHAR* SocketName, const TCHAR* BoneName)
        {
            if (USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,Path))
            {
                if (USkeletalMeshSocket* Socket=Mesh->FindSocket(SocketName))
                {
                    Socket->Modify();
                    Mesh->Modify();
                    Socket->BoneName=BoneName;
                    Mesh->MarkPackageDirty();
                    UE_LOG(LogSteppe,Display,TEXT("Fixed %s:%s -> %s"),Path,SocketName,BoneName);
                }
            }
        };
        const TCHAR* Horse=TEXT("/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/Horse.Horse");
        const TCHAR* Rider=TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin");
        Fix(Horse,TEXT("RiderSeat"),TEXT("Body"));
        Fix(Horse,TEXT("Head"),TEXT("Head"));
        Fix(Horse,TEXT("Neck"),TEXT("Neck2"));
        Fix(Horse,TEXT("Chest"),TEXT("Body"));
        Fix(Rider,TEXT("LassoHand_R"),TEXT("hand_r"));
        Fix(Rider,TEXT("Rein_L"),TEXT("hand_l"));
        Fix(Rider,TEXT("Rein_R"),TEXT("hand_r"));
#endif
    }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FSteppeModule, Steppe, "Steppe");
