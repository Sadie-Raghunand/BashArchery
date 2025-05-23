// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class WinBT : ModuleRules
{
	public WinBT(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
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


		string windowsSdkVersion = Target.WindowsPlatform.WindowsSdkVersion;
		string windowsSdkPath = Target.WindowsPlatform.WindowsSdkDir;
		
		if (!string.IsNullOrEmpty(windowsSdkVersion) && !string.IsNullOrEmpty(windowsSdkPath))
		{
			string WinRTIncludePath = Path.Combine(windowsSdkPath, "Include", windowsSdkVersion, "cppwinrt");

			if (Directory.Exists(WinRTIncludePath))
			{
				PublicSystemIncludePaths.Add(WinRTIncludePath);
			}
			else
			{
				throw new BuildException($"WinRT Include Path not found: {WinRTIncludePath}");
			}
		}

		PublicAdditionalLibraries.Add("windowsapp.lib");

		PublicSystemLibraries.Add("WindowsApp.lib");
		
	}
}
