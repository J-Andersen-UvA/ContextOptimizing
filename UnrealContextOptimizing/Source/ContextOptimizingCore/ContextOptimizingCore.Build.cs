using UnrealBuildTool;

public class ContextOptimizingCore : ModuleRules
{
    public ContextOptimizingCore(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;

        PublicDependencyModuleNames.Add("Core");
    }
}

