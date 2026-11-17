using UnrealBuildTool;
using System.Collections.Generic;

public class CoinRushTarget : TargetRules
{
	public CoinRushTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CoinRush");
	}
}
