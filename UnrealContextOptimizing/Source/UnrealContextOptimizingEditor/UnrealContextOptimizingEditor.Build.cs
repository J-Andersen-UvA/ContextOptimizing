using UnrealBuildTool;

public class UnrealContextOptimizingEditor : ModuleRules
{
    public UnrealContextOptimizingEditor(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "AdditiveBoneCorrection",
                "UnrealContextOptimizing",
                "SemanticPatchProximity",
                "SemanticSurfacePatches"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "AdditiveBoneCorrectionEditor",
                "UnrealEd",
                "AnimationBlueprintLibrary",
                "SemanticSurfacePatchesEditor"
            });
    }
}

