namespace UnrealBuildTool.Rules
{
	public class Trials : ModuleRules
	{
		public Trials(TargetInfo Target)
        {
            PrivateIncludePaths.Add("Trials/Private");

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "UnrealTournament",
					"InputCore",
					"SlateCore",
                    "Http",
                    "Json",
                    "JsonUtilities"
                }
			);
		}
	}
}