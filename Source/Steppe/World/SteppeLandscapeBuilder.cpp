#include "World/SteppeLandscapeBuilder.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Steppe.h"

bool USteppeLandscapeBuilder::BuildGrasslandBlockout()
{
#if WITH_EDITOR
    UWorld* World=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
    if (!World) { UE_LOG(LogSteppe,Error,TEXT("P16.6 landscape builder has no editor world")); return false; }
    for (TActorIterator<ALandscape> It(World);It;++It)
    {
        if (It->GetActorNameOrLabel().Contains(TEXT("P16.6")) || It->GetName().Contains(TEXT("P16_6")))
        {
            UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_LANDSCAPE: existing blockout retained"));
            return true;
        }
    }
    constexpr int32 Components=8, QuadsPerComponent=63, Quads=Components*QuadsPerComponent, Verts=Quads+1;
    constexpr float XYScale=160.f, ZScale=100.f;
    TArray<uint16> Heights; Heights.SetNumUninitialized(Verts*Verts);
    auto Gaussian=[](float X,float Y,float CX,float CY,float SX,float SY)
    {
        const float DX=(X-CX)/SX, DY=(Y-CY)/SY; return FMath::Exp(-.5f*(DX*DX+DY*DY));
    };
    for (int32 Y=0;Y<Verts;++Y) for (int32 X=0;X<Verts;++X)
    {
        const float WX=(X-Quads*.5f)*XYScale/100.f, WY=(Y-Quads*.5f)*XYScale/100.f;
        float HeightCm=35.f*FMath::Sin(WX/105.f)*FMath::Cos(WY/135.f);
        HeightCm+=560.f*Gaussian(WX,WY,40.f,285.f,250.f,72.f);
        HeightCm+=390.f*Gaussian(WX,WY,-245.f,125.f,105.f,120.f);
        HeightCm-=210.f*Gaussian(WX,WY,165.f,35.f,145.f,190.f);
        const float RiverCenter=-175.f+32.f*FMath::Sin(WX/95.f);
        HeightCm-=155.f*FMath::Exp(-.5f*FMath::Square((WY-RiverCenter)/24.f));
        HeightCm*=1.f-.72f*Gaussian(WX,WY,0.f,-315.f,155.f,85.f);
        Heights[Y*Verts+X]=static_cast<uint16>(FMath::Clamp(32768+FMath::RoundToInt(HeightCm*128.f/ZScale),0,65535));
    }
    FActorSpawnParameters Params; Params.Name=TEXT("P16_6_GameplayLandscape");
    ALandscape* Landscape=World->SpawnActor<ALandscape>(FVector(-Quads*XYScale*.5f,-Quads*XYScale*.5f,0.f),FRotator::ZeroRotator,Params);
    if (!Landscape) { UE_LOG(LogSteppe,Error,TEXT("Could not spawn P16.6 landscape")); return false; }
    Landscape->SetActorLabel(TEXT("P16.6 Gameplay Landscape"));
    Landscape->SetActorScale3D(FVector(XYScale,XYScale,ZScale));
    const FGuid Guid=FGuid::NewGuid();
    const FGuid FinalLayerGuid;
    TMap<FGuid,TArray<uint16>> HeightData; HeightData.Add(FinalLayerGuid,MoveTemp(Heights));
    TMap<FGuid,TArray<FLandscapeImportLayerInfo>> LayerData; LayerData.Add(FinalLayerGuid,{});
    Landscape->Import(Guid,0,0,Quads,Quads,1,QuadsPerComponent,HeightData,nullptr,LayerData,
        ELandscapeImportAlphamapType::Additive,TArrayView<const FLandscapeLayer>());
    Landscape->SetLandscapeGuid(Guid);
    if (UMaterialInterface* Grass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Steppe/Debug/M_PrototypeGrass.M_PrototypeGrass"))) Landscape->LandscapeMaterial=Grass;
    Landscape->UpdateAllComponentMaterialInstances();
    Landscape->MarkPackageDirty();
    UE_LOG(LogSteppe,Display,TEXT("STEPPE_P16_6_LANDSCAPE: Verts=%d SizeMeters=806 Ridge=1 Hill=1 Valley=1 RiverBed=1"),Verts);
    return true;
#else
    return false;
#endif
}
