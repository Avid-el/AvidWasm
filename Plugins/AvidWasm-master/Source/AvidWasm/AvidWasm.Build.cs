// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class AvidWasm : ModuleRules
{
	public AvidWasm(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// wasm3 库路径
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");
		string Wasm3Path = Path.Combine(ThirdPartyPath, "wasm3");
		string Wasm3IncludePath = Path.Combine(Wasm3Path, "include");
		string Wasm3LibPath = Path.Combine(Wasm3Path, "lib");
		
		PublicIncludePaths.AddRange(
			new string[] {
				Wasm3IncludePath
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add(Path.Combine(Wasm3LibPath, "Win64", "m3.lib"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(Path.Combine(Wasm3LibPath, "Mac", "libm3.a"));
		}
	}
}
