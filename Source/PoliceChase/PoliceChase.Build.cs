// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PoliceChase : ModuleRules
{
	public PoliceChase(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"PoliceChase",
			"PoliceChase/Variant_Platforming",
			"PoliceChase/Variant_Platforming/Animation",
			"PoliceChase/Variant_Combat",
			"PoliceChase/Variant_Combat/AI",
			"PoliceChase/Variant_Combat/Animation",
			"PoliceChase/Variant_Combat/Gameplay",
			"PoliceChase/Variant_Combat/Interfaces",
			"PoliceChase/Variant_Combat/UI",
			"PoliceChase/Variant_SideScrolling",
			"PoliceChase/Variant_SideScrolling/AI",
			"PoliceChase/Variant_SideScrolling/Gameplay",
			"PoliceChase/Variant_SideScrolling/Interfaces",
			"PoliceChase/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
