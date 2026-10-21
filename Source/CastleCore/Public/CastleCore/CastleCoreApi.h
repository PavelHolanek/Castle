#pragma once

// UnrealBuildTool defines CASTLECORE_API for the module. Standalone builds (the CMake tests) export nothing.
#ifndef CASTLECORE_API
#define CASTLECORE_API
#endif

// In modular (editor) builds CASTLECORE_API expands to DLLEXPORT or DLLIMPORT, which normally come from the
// engine's platform headers. The core includes none of them, so it defines them here, identically to the engine.
#ifndef DLLEXPORT
#if defined(_MSC_VER)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif
#endif

#ifndef DLLIMPORT
#if defined(_MSC_VER)
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLIMPORT __attribute__((visibility("default")))
#endif
#endif
