// Copyright Andrei Sudarikov. All Rights Reserved.

using UnrealBuildTool;

public class DruStateChart : ModuleRules
{
    public DruStateChart(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
            
        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "StructUtils",
                "GameplayTags",
            });
        
        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
            });
    }
}
