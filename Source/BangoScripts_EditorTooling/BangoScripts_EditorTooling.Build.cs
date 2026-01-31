using UnrealBuildTool;

// This module contains shared info for all other modules, such as editor colors and editor functions.
public class BangoScripts_EditorTooling : ModuleRules
{
    public BangoScripts_EditorTooling(ReadOnlyTargetRules Target) : base(Target)
    {
	    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
	    bUseUnity = true;

	    // PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
	    // bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
				"UnrealEd",
				"Projects",
				"DeveloperSettings", 
				"ModelingComponents",
            }
        );
        
        PrivateIncludePaths.AddRange(
        new string[]
	        {
				"BangoScripts_EditorTooling",
	        }
        );
    }
}