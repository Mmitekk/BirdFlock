using UnrealBuildTool;

public class BirdFlockEditor : ModuleRules
{
	public BirdFlockEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"PropertyEditor"
		});
	}
}
