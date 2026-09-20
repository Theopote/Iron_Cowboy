#include "Steppe.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
DEFINE_LOG_CATEGORY(LogSteppe);
DEFINE_LOG_CATEGORY(LogSteppeHorse);
DEFINE_LOG_CATEGORY(LogSteppeRiding);
class FSteppeModule final : public FDefaultGameModuleImpl
{
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
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
