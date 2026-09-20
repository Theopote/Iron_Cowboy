using UnrealBuildTool;
public class Steppe : ModuleRules
{
    public Steppe(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "PhysicsCore", "AudioExtensions", "Niagara", "InputCore", "EnhancedInput", "GameplayTags", "Json", "UMG", "Slate", "SlateCore" });
        PublicIncludePaths.Add(ModuleDirectory);
    }
}
