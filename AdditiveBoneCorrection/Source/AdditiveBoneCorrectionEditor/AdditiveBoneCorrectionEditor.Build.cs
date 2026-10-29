using UnrealBuildTool;

public class AdditiveBoneCorrectionEditor : ModuleRules
{
    public AdditiveBoneCorrectionEditor(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "AdditiveBoneCorrection"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "UnrealEd",
                "AssetRegistry",
                "AnimationBlueprintLibrary",
                "AnimationDataController"
            });
    }
}
