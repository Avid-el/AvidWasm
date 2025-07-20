// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AvidTest : ModuleRules
{
	public AvidTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
