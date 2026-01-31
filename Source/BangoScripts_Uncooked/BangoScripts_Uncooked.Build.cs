using UnrealBuildTool;

public class BangoScripts_Uncooked : ModuleRules
{
    public BangoScripts_Uncooked(ReadOnlyTargetRules Target) : base(Target)
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
                "BangoScripts",
                "BangoScripts_EditorTooling",

                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "BlueprintGraph",
                "UnrealEd",
                "KismetCompiler",
                "Kismet",
                "EditorSubsystem",
                "GraphEditor",
            }
        );
        
        PrivateIncludePaths.AddRange(
	        new string[]
	        {
				"BangoScripts_Uncooked",
	        }
        );
    }
}