using UnrealBuildTool;

/**
 * Plain C++ core of the wall system. Apart from CastleCoreModule.cpp nothing here includes Unreal headers,
 * so the same sources also build and run their tests without the engine (Tests/CastleCore).
 */
public class CastleCore : ModuleRules
{
	public CastleCore(ReadOnlyTargetRules Target) : base(Target)
	{
		// No precompiled header, so that nothing from the engine leaks into the core sources.
		PCHUsage = PCHUsageMode.NoPCHs;

		// Unity builds would merge the anonymous namespaces of different .cpp files.
		bUseUnity = false;

		// Only for IMPLEMENT_MODULE in CastleCoreModule.cpp.
		PrivateDependencyModuleNames.Add("Core");
	}
}
