#include "World/SteppeTerrainZone.h"
#include "Components/BoxComponent.h"
#include "Character/Horse/SteppeHorseCharacter.h"
#include "Character/Horse/HorseMovementComponent.h"

ASteppeTerrainZone::ASteppeTerrainZone()
{
    PrimaryActorTick.bCanEverTick=false;
    Volume=CreateDefaultSubobject<UBoxComponent>(TEXT("TerrainVolume"));
    SetRootComponent(Volume);
    Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
    Volume->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
    Volume->SetGenerateOverlapEvents(true);
    Volume->OnComponentBeginOverlap.AddDynamic(this,&ASteppeTerrainZone::Enter);
    Volume->OnComponentEndOverlap.AddDynamic(this,&ASteppeTerrainZone::Leave);
}
void ASteppeTerrainZone::Enter(UPrimitiveComponent*,AActor* Actor,UPrimitiveComponent*,int32,bool,const FHitResult&)
{
    if (auto* Horse=Cast<ASteppeHorseCharacter>(Actor))
    {
        if (auto* Move=Cast<UHorseMovementComponent>(Horse->GetCharacterMovement())) Move->SurfaceMovementMultiplier=MovementScale;
    }
}
void ASteppeTerrainZone::Leave(UPrimitiveComponent*,AActor* Actor,UPrimitiveComponent*,int32)
{
    if (auto* Horse=Cast<ASteppeHorseCharacter>(Actor))
    {
        if (auto* Move=Cast<UHorseMovementComponent>(Horse->GetCharacterMovement())) Move->SurfaceMovementMultiplier=1.f;
    }
}
