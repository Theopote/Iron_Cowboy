#include "Input/SteppeInputConfig.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
void USteppeInputConfig::CreateRuntimeDefaults()
{
    auto MakeAction=[this](const TCHAR* Name,EInputActionValueType Type)
    {
        UInputAction* Action=NewObject<UInputAction>(this,FName(Name)); Action->ValueType=Type; return Action;
    };
    Move=MakeAction(TEXT("IA_Move"),EInputActionValueType::Axis2D);
    Look=MakeAction(TEXT("IA_Look"),EInputActionValueType::Axis2D);
    Sprint=MakeAction(TEXT("IA_Sprint"),EInputActionValueType::Boolean);
    Brake=MakeAction(TEXT("IA_Brake"),EInputActionValueType::Boolean);
    Interact=MakeAction(TEXT("IA_Interact"),EInputActionValueType::Boolean);
    MountDismount=MakeAction(TEXT("IA_MountDismount"),EInputActionValueType::Boolean);
    Debug=MakeAction(TEXT("IA_Debug"),EInputActionValueType::Boolean);
    FocusTarget=MakeAction(TEXT("IA_FocusTarget"),EInputActionValueType::Boolean);
    RestartTrial=MakeAction(TEXT("IA_RestartTrial"),EInputActionValueType::Boolean);
    OnFoot=NewObject<UInputMappingContext>(this,TEXT("IMC_OnFoot"));
    Riding=NewObject<UInputMappingContext>(this,TEXT("IMC_Riding"));
    for (UInputMappingContext* Context : {OnFoot.Get(),Riding.Get()})
    {
        auto Axis=[Context](UInputAction* Action,FKey Key,bool Negative,bool Y)
        {
            auto& Mapping=Context->MapKey(Action,Key);
            if (Negative) { Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context)); }
            if (Y) { auto* Swizzle=NewObject<UInputModifierSwizzleAxis>(Context); Swizzle->Order=EInputAxisSwizzle::YXZ; Mapping.Modifiers.Add(Swizzle); }
        };
        Axis(Move,EKeys::W,false,true); Axis(Move,EKeys::S,true,true);
        Axis(Move,EKeys::D,false,false); Axis(Move,EKeys::A,true,false);
        Axis(Look,EKeys::MouseX,false,false); Axis(Look,EKeys::MouseY,true,true);
        Context->MapKey(Sprint,EKeys::LeftShift); Context->MapKey(Brake,EKeys::LeftControl);
        Context->MapKey(MountDismount,EKeys::E); Context->MapKey(FocusTarget,EKeys::Q);
        Context->MapKey(Debug,EKeys::F1); Context->MapKey(RestartTrial,EKeys::F2);
    }
}
