// Copyright Andrei Sudarikov. All Rights Reserved.

using UnrealBuildTool;

public class DruStateChartEditor : ModuleRules
{
    public DruStateChartEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                //"UMG",
            });
        
        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "EditorStyle",
                "AssetTools",
                //"BlueprintGraph",
                //"GraphEditor",
                //"UMGEditor",
                //"KismetCompiler",
                "UnrealEd",
                "DruStateChart",
            });
    }
}
