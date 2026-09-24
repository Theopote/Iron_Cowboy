#include "Steppe.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AnimationGraph.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "K2Node_VariableGet.h"
#include "Presentation/RiderAnimInstance.h"
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
        if (FParse::Param(FCommandLine::Get(),TEXT("SteppeBuildRiderContactGraph")))
        {
            UAnimBlueprint* Blueprint=LoadObject<UAnimBlueprint>(nullptr,
                TEXT("/Game/Steppe/Presentation/ABP_Rider.ABP_Rider"));
            if (!Blueprint) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimBlueprint missing")); return; }
            TArray<UEdGraph*> Graphs;
            Blueprint->GetAllGraphs(Graphs);
            UAnimationGraph* Graph=nullptr;
            for (UEdGraph* Candidate : Graphs)
            {
                if (Candidate->GetName()==TEXT("AnimGraph")) { Graph=Cast<UAnimationGraph>(Candidate); break; }
            }
            if (!Graph) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimGraph missing")); return; }
            UAnimGraphNode_Root* Root=nullptr;
            UAnimGraphNode_LayeredBoneBlend* Blend=nullptr;
            int32 ExistingIK=0;
            TArray<UAnimGraphNode_TwoBoneIK*> ExistingIKNodes;
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (auto* Candidate=Cast<UAnimGraphNode_Root>(Node)) { Root=Candidate; }
                if (auto* Candidate=Cast<UAnimGraphNode_LayeredBoneBlend>(Node)) { Blend=Candidate; }
                if (auto* IK=Cast<UAnimGraphNode_TwoBoneIK>(Node)) { ++ExistingIK; ExistingIKNodes.Add(IK); }
            }
            if (!Root || !Blend) { UE_LOG(LogSteppe,Error,TEXT("Rider layered graph incomplete")); return; }
            if (ExistingIK>0)
            {
                for (UAnimGraphNode_TwoBoneIK* IK : ExistingIKNodes)
                {
                    FVector KneeTarget=FVector::ZeroVector;
                    if (IK->Node.IKBone.BoneName==TEXT("foot_l"))
                    {
                        // The mannequin mesh is yawed -90 degrees on the rider. In
                        // component space +X is the rider's left, so the old -X
                        // target folded this knee inward across the horse.
                        KneeTarget=FVector(72.f,22.f,90.f);
                    }
                    else if (IK->Node.IKBone.BoneName==TEXT("foot_r"))
                    {
                        KneeTarget=FVector(-72.f,22.f,90.f);
                    }
                    if (!KneeTarget.IsZero())
                    {
                        IK->Node.JointTargetLocation=KneeTarget;
                        for (UEdGraphPin* Pin : IK->Pins)
                        {
                            if (Pin && Pin->PinName==TEXT("JointTargetLocation"))
                            {
                                UE_LOG(LogSteppe,Display,TEXT("Rider knee pin %s old=%s"),*IK->Node.IKBone.BoneName.ToString(),*Pin->DefaultValue);
                                Pin->DefaultValue=FString::Printf(TEXT("%f,%f,%f"),KneeTarget.X,KneeTarget.Y,KneeTarget.Z);
                            }
                        }
                    }
                }
                Graph->NotifyGraphChanged();
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                FKismetEditorUtilities::CompileBlueprint(Blueprint);
                Blueprint->MarkPackageDirty();
                UE_LOG(LogSteppe,Display,TEXT("Rider contact knee targets corrected nodes=%d"),ExistingIK);
                return;
            }
            auto PosePin=[](UEdGraphNode* Node,EEdGraphPinDirection Direction) -> UEdGraphPin*
            {
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin && Pin->Direction==Direction && Pin->PinType.PinCategory==TEXT("struct")
                        && (Pin->PinName==TEXT("Pose") || Pin->PinName==TEXT("ComponentPose"))) { return Pin; }
                }
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin && Pin->Direction==Direction && Pin->PinType.PinCategory==TEXT("struct")
                        && Pin->PinType.PinSubCategoryObject.IsValid()
                        && Pin->PinType.PinSubCategoryObject->GetName().Contains(TEXT("PoseLink"))) { return Pin; }
                }
                return nullptr;
            };
            auto NamedPin=[](UEdGraphNode* Node,EEdGraphPinDirection Direction,FName Name) -> UEdGraphPin*
            {
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin && Pin->Direction==Direction && Pin->PinName==Name) { return Pin; }
                }
                return nullptr;
            };
            FGraphNodeCreator<UAnimGraphNode_LocalToComponentSpace> ToComponentCreator(*Graph);
            auto* ToComponent=ToComponentCreator.CreateNode();
            ToComponent->NodePosX=Blend->NodePosX+240; ToComponent->NodePosY=Blend->NodePosY;
            ToComponentCreator.Finalize();
            FGraphNodeCreator<UAnimGraphNode_ComponentToLocalSpace> ToLocalCreator(*Graph);
            auto* ToLocal=ToLocalCreator.CreateNode();
            ToLocal->NodePosX=Blend->NodePosX+1250; ToLocal->NodePosY=Blend->NodePosY;
            ToLocalCreator.Finalize();
            const UEdGraphSchema* Schema=Graph->GetSchema();
            UEdGraphPin* Previous=PosePin(ToComponent,EGPD_Output);
            bool bLinks=Schema->TryCreateConnection(PosePin(Blend,EGPD_Output),PosePin(ToComponent,EGPD_Input));
            const FName Bones[3]={TEXT("foot_l"),TEXT("foot_r"),TEXT("hand_l")};
            const FName Targets[3]={TEXT("LeftFootTarget"),TEXT("RightFootTarget"),TEXT("LeftReinTarget")};
            for (int32 Index=0;Index<3;Index++)
            {
                FGraphNodeCreator<UAnimGraphNode_TwoBoneIK> IKCreator(*Graph);
                auto* IK=IKCreator.CreateNode();
                IK->NodePosX=Blend->NodePosX+490+Index*250;
                IK->NodePosY=Blend->NodePosY;
                IK->Node.IKBone.BoneName=Bones[Index];
                IK->Node.EffectorLocationSpace=BCS_ComponentSpace;
                IK->Node.JointTargetLocationSpace=BCS_ComponentSpace;
                IK->Node.JointTargetLocation=Index==0?FVector(72.f,22.f,90.f):
                    (Index==1?FVector(-72.f,22.f,90.f):FVector(0.f,70.f,35.f));
                IK->Node.bAllowStretching=false;
                IKCreator.Finalize();
                IK->ReconstructNode();
                bLinks&=Previous && Schema->TryCreateConnection(Previous,PosePin(IK,EGPD_Input));
                Previous=PosePin(IK,EGPD_Output);
                for (FName Variable : {Targets[Index],FName(TEXT("ContactAlpha"))})
                {
                    FGraphNodeCreator<UK2Node_VariableGet> VariableCreator(*Graph);
                    auto* Getter=VariableCreator.CreateNode();
                    Getter->VariableReference.SetSelfMember(Variable);
                    Getter->NodePosX=IK->NodePosX-100;
                    Getter->NodePosY=IK->NodePosY+(Variable==Targets[Index]?230:340);
                    VariableCreator.Finalize();
                    UEdGraphPin* Source=NamedPin(Getter,EGPD_Output,Variable);
                    UEdGraphPin* Target=NamedPin(IK,EGPD_Input,Variable==Targets[Index]?FName(TEXT("EffectorLocation")):FName(TEXT("Alpha")));
                    bLinks&=Source && Target && Schema->TryCreateConnection(Source,Target);
                }
            }
            bLinks&=Previous && Schema->TryCreateConnection(Previous,PosePin(ToLocal,EGPD_Input));
            UEdGraphPin* RootIn=PosePin(Root,EGPD_Input);
            if (RootIn) { RootIn->BreakAllPinLinks(); }
            bLinks&=RootIn && Schema->TryCreateConnection(PosePin(ToLocal,EGPD_Output),RootIn);
            UE_LOG(LogSteppe,Display,TEXT("Rider contact graph links=%d"),bLinks);
            if (!bLinks) { return; }
            Graph->NotifyGraphChanged();
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);
            Blueprint->MarkPackageDirty();
            return;
        }
        if (FParse::Param(FCommandLine::Get(),TEXT("SteppeBuildRiderLayeredGraph")))
        {
            UAnimBlueprint* Blueprint=LoadObject<UAnimBlueprint>(nullptr,
                TEXT("/Game/Steppe/Presentation/ABP_Rider.ABP_Rider"));
            if (!Blueprint) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimBlueprint missing")); return; }
            TArray<UEdGraph*> Graphs;
            Blueprint->GetAllGraphs(Graphs);
            UAnimationGraph* Graph=nullptr;
            for (UEdGraph* Candidate : Graphs)
            {
                if (Candidate->GetName()==TEXT("AnimGraph")) { Graph=Cast<UAnimationGraph>(Candidate); break; }
            }
            if (!Graph) { UE_LOG(LogSteppe,Error,TEXT("Rider AnimGraph missing")); return; }
            UAnimGraphNode_Root* Root=nullptr;
            UAnimGraphNode_Slot* BaseSlot=nullptr;
            bool bAlreadyBuilt=false;
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (auto* Candidate=Cast<UAnimGraphNode_Root>(Node)) { Root=Candidate; }
                if (auto* Candidate=Cast<UAnimGraphNode_Slot>(Node); Candidate && Candidate->Node.SlotName==TEXT("DefaultSlot")) { BaseSlot=Candidate; }
                bAlreadyBuilt|=Node->IsA<UAnimGraphNode_LayeredBoneBlend>();
            }
            if (!Root || !BaseSlot) { UE_LOG(LogSteppe,Error,TEXT("Rider base slot or root missing")); return; }
            if (USkeleton* Skeleton=Blueprint->TargetSkeleton)
            {
                Skeleton->RegisterSlotNode(TEXT("UpperBodySlot"));
                Skeleton->AddSlotGroupName(TEXT("SteppeUpperBody"));
                Skeleton->SetSlotGroupName(TEXT("UpperBodySlot"),TEXT("SteppeUpperBody"));
                Skeleton->MarkPackageDirty();
            }
            if (!bAlreadyBuilt)
            {
                FGraphNodeCreator<UAnimGraphNode_Slot> SlotCreator(*Graph);
                UAnimGraphNode_Slot* UpperSlot=SlotCreator.CreateNode();
                UpperSlot->NodePosX=BaseSlot->NodePosX+230;
                UpperSlot->NodePosY=BaseSlot->NodePosY+220;
                UpperSlot->Node.SlotName=TEXT("UpperBodySlot");
                SlotCreator.Finalize();
                FGraphNodeCreator<UAnimGraphNode_LayeredBoneBlend> BlendCreator(*Graph);
                UAnimGraphNode_LayeredBoneBlend* Blend=BlendCreator.CreateNode();
                Blend->NodePosX=BaseSlot->NodePosX+480;
                Blend->NodePosY=BaseSlot->NodePosY;
                BlendCreator.Finalize();
                if (!Blend->Node.LayerSetup.IsValidIndex(0)) { UE_LOG(LogSteppe,Error,TEXT("Layered blend has no pose")); return; }
                FBranchFilter Filter;
                Filter.BoneName=TEXT("spine_01");
                Filter.BlendDepth=0;
                Blend->Node.LayerSetup[0].BranchFilters.Add(Filter);
                Blend->Node.BlendWeights[0]=1.f;
                Blend->ReconstructNode();
                auto PosePin=[](UEdGraphNode* Node,EEdGraphPinDirection Direction,const TCHAR* Name=nullptr) -> UEdGraphPin*
                {
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction==Direction && Pin->PinType.PinCategory==TEXT("struct")
                            && (!Name || Pin->PinName.ToString().StartsWith(Name))) { return Pin; }
                    }
                    return nullptr;
                };
                UEdGraphPin* BaseOut=PosePin(BaseSlot,EGPD_Output);
                UEdGraphPin* RootIn=PosePin(Root,EGPD_Input);
                UEdGraphPin* BlendBase=PosePin(Blend,EGPD_Input,TEXT("BasePose"));
                UEdGraphPin* BlendUpper=PosePin(Blend,EGPD_Input,TEXT("BlendPoses"));
                UEdGraphPin* BlendOut=PosePin(Blend,EGPD_Output);
                UEdGraphPin* UpperIn=PosePin(UpperSlot,EGPD_Input);
                UEdGraphPin* UpperOut=PosePin(UpperSlot,EGPD_Output);
                if (!BaseOut || !RootIn || !BlendBase || !BlendUpper || !BlendOut || !UpperIn || !UpperOut)
                {
                    UE_LOG(LogSteppe,Error,TEXT("Rider layered pose pins missing")); return;
                }
                RootIn->BreakAllPinLinks();
                const UEdGraphSchema* Schema=Graph->GetSchema();
                const bool bLinks=Schema->TryCreateConnection(BaseOut,BlendBase)
                    && Schema->TryCreateConnection(BaseOut,UpperIn)
                    && Schema->TryCreateConnection(UpperOut,BlendUpper)
                    && Schema->TryCreateConnection(BlendOut,RootIn);
                UE_LOG(LogSteppe,Display,TEXT("Rider layered graph links=%d"),bLinks);
                if (!bLinks) { return; }
                Graph->NotifyGraphChanged();
                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                FKismetEditorUtilities::CompileBlueprint(Blueprint);
                Blueprint->MarkPackageDirty();
            }
            UE_LOG(LogSteppe,Display,TEXT("Rider layered graph ready nodes=%d"),Graph->Nodes.Num());
            return;
        }
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
