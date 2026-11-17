using UnrealBuildTool;

public class CoinRush : ModuleRules
{
	public CoinRush(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 源文件平铺在模块根目录（没有 Public/Private 分层），
		// 这样 Private/Tests 下的文件也能 #include "CRRoundModel.h"。
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore"
		});
	}
}
