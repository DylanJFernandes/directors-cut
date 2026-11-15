// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DirectorsCut : ModuleRules
{
	public DirectorsCut(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"DirectorsCut",
			ModuleDirectory + "/Public"
		});
	}
}
