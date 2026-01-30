using UnrealBuildTool;

public class BangoScripts_Editor : ModuleRules
{
    public BangoScripts_Editor(ReadOnlyTargetRules Target) : base(Target)
    {
	    PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
		
	    bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "Kismet",
            }
        );
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "BangoScripts",
                "BangoScripts_Uncooked",
				"BangoScripts_EditorTooling",

                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "DeveloperSettings",
                "Projects",
                "InputCore",
                "UnrealEd",
                "BlueprintGraph",
                "GraphEditor",
                "ToolMenus",
                "KismetWidgets",
                "DataValidation",
                "EditorSubsystem",
				"ContentBrowser",
				"ContentBrowserData",
				"AssetDefinition",
				"ClassViewer",
				"TypedElementFramework",
				"AssetTools",
				"UMGEditor",
				"ApplicationCore",
				"PropertyEditor",
				"ToolWidgets",
				"EditorWidgets", 
				"GameProjectGeneration",
				"AssetRegistry",
				"RenderCore", 
				"ModelingComponents",
				"LevelEditor"
            }
        );
        
        PrivateIncludePaths.AddRange(
			new string[]
			{
				"BangoScripts_Editor",
			}
        );
    }
}