using UnrealBuildTool;
public class Steppe : ModuleRules
{
    public Steppe(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "AudioExtensions", "InputCore", "EnhancedInput", "GameplayTags", "UMG", "Slate", "SlateCore" });
        PublicIncludePaths.Add(ModuleDirectory);
    }
}
