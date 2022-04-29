// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PakPractice : ModuleRules
{
	public PakPractice(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay"});
		
		PrivateDependencyModuleNames.AddRange(new string[] { "PakFile" });
	}
}
