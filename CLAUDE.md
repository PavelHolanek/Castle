# Castle

A medieval castle builder and simulator in Unreal Engine 5.8 (C++20).

## Vision

- **Goals:** learning game development, with the aim of eventually releasing the game as an indie title (e.g. on Steam).
  Keep code release-worthy, not throwaway.
- **Core: free-form castle building.** Players draw their own walls instead of placing ready-made assets. It works like
  the roads in Cities Skylines: any angle, different widths, several walls meeting at a node, snapping, later guide lines.
  The author is fascinated by medieval fortifications and wants to model them in the detail they want.
- **Inspiration:** mainly German castles, which follow the terrain and have irregular, non-rectangular floor plans.
- **Period:** 12th to early 15th century, the classic age of castles, including the dawn of firearms. Influences include
  Kingdom Come: Deliverance and early bastion fortifications such as Žižka's Kalich castle (dry-laid stone bastions).
- **Gameplay** beyond building will come through several game modes, with an economy for sure. Walls define rooms;
  the player assigns a room a function (living quarters, smithy, kitchen…) and upgrades. A room is a space the player
  dedicates, not a fixed building.
- **Scale:** the design should handle castles with a few thousand walls and other elements.
- **Look:** heading towards realism, but the author is a programmer, not an artist. Placeholders for now; real art
  later, possibly from a hired artist.
- **Platforms:** PC only, Windows and Linux.
- Not handled yet: terrain and verticality (floors, wall walks).

The author is an experienced C++ programmer and new to game development and Unreal Engine; explain UE specifics.

## Architecture

```
input (UE) -> WallPlacement (core) -> WallNetwork (authoritative model) -> Junction -> FloorPlan -> lines / 3D mesh (UE)
```

- **`Source/CastleCore/`**: plain C++20 core, namespace `Castle::Core`, **no Unreal headers**. The only exception is
  `CastleCoreModule.cpp`, the module registration. The same sources build in the CMake test project, which keeps the
  core clean.
  - `Vec2.h`: vector, tolerances, angles.
  - `WallCurve`: straight line or circular arc (bulge), exact offsets, projection of a point, tessellation.
  - `WallNetwork`: nodes and walls with stable ids. Walls meet only at shared nodes; splitting a wall, finding the wall
    under a point.
  - `WallLimits`: limits (length, width, arcs, smallest joint angle 30°).
  - `Junction`: the junction solver, gaps between neighbouring walls at a node.
  - `FloorPlan`: splits the floor area into wall bodies and node pieces.
  - `Polygon`: area, regular polygon, clipping to a convex shape.
  - `WallPlacement`: state of the build tool (preview, chaining, snapping).
  - `WallOutline`: raw outlines for debugging.
- **`Source/Castle/`**: the Unreal layer. Turns input into core calls, draws, and holds the network in the world.
  - `CastleWallSubsystem`: the world's wall network and the view mode.
  - `CastleWallNetworkActor`: draws the network in the game.
  - `CastleWallDrawing`: debug lines via `ULineBatchComponent`.
  - `CastleWallMesh`: extrudes the floor plan to 3D via `UDynamicMeshComponent`.
  - `CastleWallBuildComponent`: the build tool on the player controller.
  - `CastleBuildToolbarWidget`: bottom toolbar with the wall types.
  - `CastleViewModeWidget`: Raw / Floor plan / Walls 3D switch, not in Shipping builds.
  - `CastleWallDebugActor`: fixed test cases whose nodes can be dragged in the editor.
  - `CastleCameraPawn`: the strategy camera.
- **`Tests/CastleCore/`**: GoogleTest, built without the engine.
- **`Docs/Backlog.md`**: known weaknesses and ideas for later.

## Domain rules (designed by the author; don't change without agreement)

- **Wall:** a reference line between two nodes, `WidthLeft`/`WidthRight` relative to the direction A→B, `Bulge` as in
  DXF (0 = straight, positive = counterclockwise).
- **Junctions:** the walls at a node are sorted counterclockwise and each gap between neighbours is resolved on its own,
  by its angle:
  - **up to 90°:** the sides' crossing;
  - **90–180°:** the crossing, or a "step" when thinner < −wider·cos θ. Then the wider wall's side is extended, by at
    most 2 × its width; a longer one gets beveled, and it never reaches past the middle of the thinner wall's side;
  - **180°:** a bevel when the widths differ;
  - **over 180°:** the bevel goes on until `cos(θ − 180°) = thinner/wider`, then the node's corner style decides:
    sharp (crossing) or beveled (joining line).
- **Floor plan:** at a node, each wall is cut perpendicular to its axis through the junction's attach point farthest
  from the node. Everything between the cuts is the **node piece**, owned by the node rather than any wall. Nodes will
  get their own features later (palisade post, higher crenellation section, material transitions). A free end gets a
  perpendicular cap.
- **Outside the limits:**
  - in the line views the node stays unresolved and its walls end in caps;
  - in 3D the joint turns red, clipped by a circle of 2 × the widest wall's width;
  - a wall consumed by the joints at both ends is only flagged (collisions will be handled later).
- **The cursor** always points at the ground plane, even over walls. Deliberate, not a bug.

## Conventions

- **Coordinates:** the core uses a right-handed plane (left and counterclockwise as on paper); Unreal's XY seen from
  above is mirrored. The mapping is Unreal (X, Y) ↔ core (X, −Y), in `CastleCoreConversion.h`. Units are cm, type
  `double`. Polygons are counterclockwise.
- **Core under UBT:** `NoPCHs`, `bUseUnity = false`, no exceptions or RTTI, `/W4 /WX`. Put `CASTLECORE_API` on
  individual functions, not classes (C4251). `CastleCoreApi.h` defines `DLLEXPORT`/`DLLIMPORT`.
- **Portability (Linux):** standard C++ only in the core (the CMake project also has GCC/Clang flags), no
  Windows-specific APIs, and include paths must match file names' case exactly.
- **Style:** like Unreal. PascalCase, `b` prefix for bools, `F`/`U`/`A`/`E` prefixes in Unreal code, core types without
  prefixes. Tabs, English comments, sparing.
- **UI and input** are built in C++ (no Widget Blueprints, Enhanced Input actions created in code). Debug-only features
  go behind `#if !UE_BUILD_SHIPPING`.

## Building and testing

- **Core tests** (CMake isn't on PATH; it comes with Visual Studio):
  ```
  CMAKE="C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
  "$CMAKE/cmake.exe" -S Tests/CastleCore -B Tests/CastleCore/build
  "$CMAKE/cmake.exe" --build Tests/CastleCore/build --config Debug
  "$CMAKE/ctest.exe" --test-dir Tests/CastleCore/build -C Debug --output-on-failure
  ```
- **CI:** `.github/workflows/core-tests.yml` runs the core tests on GitHub on Windows (MSVC) and Linux (GCC, Clang)
  for every push that touches the core. Linux is case sensitive and GCC/Clang are stricter, so a core change isn't
  finished until it passes there too. Only the core is checked; the Unreal layer isn't built on Linux yet.
- **Unreal build:** always the **`CastleEditor`** target. The `Castle` game target is monolithic and hides DLL export
  errors.
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" CastleEditor Win64 Development "-Project=C:\Users\Pavel\Documents\Unreal Projects\Castle\Castle.uproject" -WaitMutex
  ```
  Look for warnings in `%LOCALAPPDATA%\UnrealBuildTool\Log.txt` (`warning C`, `error C`). The build should have none.
- **Before building**, check whether `UnrealEditor` or `LiveCodingConsole` is running. If so, don't build; ask the
  author to close the editor (they often forget). Live Coding can't handle new classes or `UPROPERTY` changes. Never
  close the editor yourself.

## Working together

- **Talk to the author in Czech**; they prefer it, though English is fine too. Code, comments, docs, the backlog and
  in-game text are in English.
- **The author designs the geometry rules**, often with a sketch. Proceed step by step: implement one case, give the
  expected values for the debug actor (computed independently, e.g. with a Python script in the scratchpad), wait for
  verification in the editor. Don't jump ahead to cases deferred "for later".
- Check the author's formulas against the geometry before implementing. If they disagree, say so with a numeric
  counterexample (it happened: −W/cos θ instead of −W·cos θ).
- After each step, say exactly what to try in the game or the editor.
- Record deferred problems and ideas in `Docs/Backlog.md`; remove them once done.
- **Git is the author's.** Don't run anything that changes the repository (commit, branch, stash, push…) unless asked
  explicitly. Suggesting a commit is fine.

## Status

Done:
- wall model, junction solver for straight walls at all angles, floor plan with node pieces;
- limits; wall placement with preview, chaining and snapping to nodes (radius grows with the widest wall) and to
  walls, which get split (T); ending on a node or a wall ends the chain;
- bottom toolbar with three wall widths, view mode switch;
- the first, debug wall type: extruded to 6 m, one color.

Next candidates from the backlog: splitting crossed walls (X), arc junctions, deleting walls and undo, wall types in the
model, wall collisions, random junction tests.
