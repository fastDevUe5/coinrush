using UnrealBuildTool;
using System.Collections.Generic;

public class CoinRushEditorTarget : TargetRules
{
	public CoinRushEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CoinRush");
	}
}
