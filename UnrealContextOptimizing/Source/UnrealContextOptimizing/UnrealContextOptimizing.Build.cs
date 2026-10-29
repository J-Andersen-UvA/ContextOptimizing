using UnrealBuildTool;

public class UnrealContextOptimizing : ModuleRules
{
    public UnrealContextOptimizing(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "ContextOptimizingCore",
                "SemanticSurfacePatches",
                "SemanticPatchProximity"
            });
    }
}

