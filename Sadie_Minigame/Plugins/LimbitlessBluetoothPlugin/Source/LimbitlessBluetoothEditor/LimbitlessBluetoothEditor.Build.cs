using UnrealBuildTool;

public class LimbitlessBluetoothEditor : ModuleRules
{
    public LimbitlessBluetoothEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "Blutility",
                "UnrealEd",
                "EditorSubsystem",
                "LimbitlessBluetoothPlugin", 
                "PlatformManager",
            }
        );
    }
}