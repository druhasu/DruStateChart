// Copyright Andrei Sudarikov. All Rights Reserved.

using UnrealBuildTool;

public class DruStateChartTests : ModuleRules
{
	public DruStateChartTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"DruStateChart",
				"StructUtils",
			});
	}
}
