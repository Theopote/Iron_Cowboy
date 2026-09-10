#include "Character/Horse/HorseLocomotionConfig.h"
UHorseLocomotionConfig::UHorseLocomotionConfig()
{
    Gaits = {{0,180,100,1,-1.5f}, {180,180,110,1,-1}, {400,220,120,1,-.7f},
             {750,260,140,1,-.3f}, {1200,300,160,1,.12f}, {1500,250,180,1,1}};
    const float X[] = {0,.3f,.6f,.8f,1};
    const float Y[] = {1,.9f,.65f,.4f,.22f};
    for (int32 I=0; I<5; ++I) { SpeedTurnCurve.GetRichCurve()->AddKey(X[I],Y[I]); }
    const float Speeds[] = {0,180,400,750,1200,1500};
    const float FOV[] = {69,70,71,73,75,78};
    const float Distance[] = {350,350,370,400,430,460};
    for (int32 I=0; I<6; ++I)
    {
        SpeedFOVCurve.GetRichCurve()->AddKey(Speeds[I], FOV[I]);
        SpeedDistanceCurve.GetRichCurve()->AddKey(Speeds[I], Distance[I]);
    }
}
const FHorseGaitSettings& UHorseLocomotionConfig::GetGait(EHorseGait Gait) const
{
    const int32 Index = FMath::Clamp(static_cast<int32>(Gait), 0, 5);
    if (Gaits.IsValidIndex(Index)) { return Gaits[Index]; }
    // Malformed designer arrays still have a usable baseline.
    static const FHorseGaitSettings Fallback(180,180,120,1,-1);
    return Fallback;
}
