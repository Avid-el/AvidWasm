// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class AvidWasm : ModuleRules
{
	public AvidWasm(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bool bUseWasm3 = false; // 是否使用 wasm3 库
		bool bUseWasmtime = true; // 是否使用 wasmtime 库

		string WasmIncludePath = "";
		string WasmLibPath = "";
		if (bUseWasm3)
		{
			PublicDefinitions.Add("WITH_WASM3=1");
			// wasm3 库路径
			string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");
			string WasmRuntimePath = Path.Combine(ThirdPartyPath, "wasm3");
			WasmIncludePath = Path.Combine(WasmRuntimePath, "include");
			WasmLibPath = Path.Combine(WasmRuntimePath, "lib");
		}
		else if (bUseWasmtime)
		{
			PublicDefinitions.Add("WITH_WASMTIME=1");
			// wasm3 库路径
			string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");
			string WasmRuntimePath = Path.Combine(ThirdPartyPath, "Wasmtime");
			WasmIncludePath = Path.Combine(WasmRuntimePath, "include");
			WasmLibPath = Path.Combine(WasmRuntimePath, "lib");
		}


		PublicIncludePaths.AddRange(
			new string[] {
				WasmIncludePath
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
			PublicAdditionalLibraries.Add(Path.Combine(WasmLibPath, "Win64", "m3.lib"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			if (bUseWasmtime)
			{
				PublicAdditionalLibraries.Add(Path.Combine(WasmLibPath, "Mac", "libwasmtime.a"));
			}
			else if (bUseWasm3)
			{
				PublicAdditionalLibraries.Add(Path.Combine(WasmLibPath, "Mac", "libm3.a"));
			}
		}
	}
}
