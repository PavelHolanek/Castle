# Backlog

Things that came up during development and haven't been dealt with yet: known weaknesses, possible bugs and ideas.
Remove items once done; decisions move into the code or the comments next to it.

## Known weaknesses

### Junctions (`Source/CastleCore/Private/Junction.cpp`)

- **Joints colliding on a short wall.** The cuts from both ends may meet or pass each other; nothing is left of the
  wall and the two node pieces may overlap. The floor plan only flags it (`WallBody::bConsumedByJoints`, magenta axis
  in the drawing). Can't be solved in the core alone; the build tool will need restrictions.
- **A corner without an extension has no stop.** An extension ends at the middle of the thinner wall's side at most,
  but a plain crossing of the sides may lie beyond the end of a short wall. Example: max width 5 m, min length 1 m,
  two walls at 90° → corner 2.5 m from the node. The wall's cut then lies past its end and the wall is "consumed"
  (see the previous item).
- **Maximum extension length** is 2 × the wider wall's width on that side for now
  (`JunctionSettings::MaxExtensionFactor`). A provisional value, to be checked in practice.
- **Long spike of a sharp outer corner.** The spike's distance from the node grows with 1 / sin(θ / 2), θ being the
  inner angle. At 30° it is about 3.9 × the half width. Consider beveling it automatically beyond some length.
- **Arcs** aren't handled in junctions yet (a gap next to an arc stays `Unresolved`).
- **Boundary angles.** Exactly at the boundary between two rules the tolerance decides. The outline is the same either
  way, but the path points may differ (e.g. two nearly identical points).
- **Free wall end** ends in the floor plan with a perpendicular cap at the node. Other shapes (rounded, beveled) don't
  exist yet.
- **Wall precedence in 3D.** The rule "the wider wall gets extended" is purely 2D. With heights, precedence will likely
  decide (tower over curtain wall, higher wall over lower), and it need not match the width.
- **Extension factor below 1.** The end of the thinner wall's side always lies closer than the wider wall's width, so
  from a factor of 1 it is always in reach (the comment in `ExtendSide` needlessly talks about 2). Below 1,
  `IntersectRayWithCircle` may return the farther of the two circle crossings. The debug actor allows factors from 0.
  Either restrict the factor to ≥ 1, or decide which crossing to take.

### Floor plan (`Source/CastleCore/Private/FloorPlan.cpp`)

- **Large node piece at sharp angles.** The cut runs through the farthest attach point, at a sharp angle through the
  crossing of the inner sides. At 30° and equal widths that is about 3.7 × the half width from the node, and the whole
  piece belongs to the node. Check whether it gets in the way of drawing node features (higher crenellation section,
  palisade post).
- **Joint outside the limits in 3D** is clipped by a circle (2 × the widest wall's width) and stands 5 cm higher than
  the walls, which end beneath it in raw caps and overlap. The top faces of overlapping grey walls lie at the same
  height; not visible yet, but with a texture they might flicker (z-fighting).
- **Node with an arc** stays unresolved: its walls end in perpendicular caps and overlap (`FloorPlan::UnresolvedNodes`).
  The same goes for nodes outside the limits, which the game passes as skipped.
- **The node piece is simplified** (points on the line between their neighbours are removed). The points where a wall
  side joins the gap path may therefore be missing from the polygon. Once drawing a node needs to know which part of
  the outline belongs to which wall, it will need a richer description than a bare polygon.

### Wall network (`Source/CastleCore/Public/CastleCore/WallNetwork.h`)

- **Overlaps away from nodes.** Junctions only handle walls sharing a node. Two walls without a common node may overlap
  thanks to their width (a wall running close past another node, two parallel walls closer than their widths). The
  network doesn't enforce "walls meet only at nodes"; it accepts walls crossing without a node. The build tool splits
  a wall that a new wall starts or ends on (T), but not yet one it crosses (X). It will also need a minimum clearance
  check, or the floor plans will have to be merged.
- **Two straight walls between the same nodes** overlap completely. The network allows them because of a pair of arcs
  (round tower); straight duplicates should be rejected.
- **Nodes at the same spot and zero-length walls.** The wall's direction comes out zero and the angles at the node make
  no sense. The limits catch it (`TooShort`), the network accepts it. The build tool should merge nearby nodes.
- **Input checks.** `AddWall` lets a NaN width through (`< 0.0` is false for NaN); neither `Bulge` nor node positions
  are checked for finite values. Important by the time saved games get loaded.
- **Pointer validity.** Pointers and spans from `FindNode`, `FindWall` and `GetNodeWalls` become invalid after
  `AddNode`, `AddWall`, `SplitWall` and `RemoveWall`. Not documented yet.

### Limits (`Source/CastleCore/Public/CastleCore/WallLimits.h`)

- The values are only a proposal.
- Relations between limits aren't checked (e.g. minimum wall length against the width of neighbouring walls).
- **The joint angle is computed twice.** `GetSmallestJointAngle` takes angles pairwise, the junctions from sorted
  directions, and a wall's direction away from a node is written out in both `WallLimits.cpp` and `Junction.cpp`.
  Unify (minimum of `Junction::Gaps[i].Angle`, or a helper in `WallNetwork`). The comment at the loop in
  `GetSmallestJointAngle` is misleading: neighbourhood isn't used.

### Curves (`Source/CastleCore/Public/CastleCore/WallCurve.h`)

- **Overflow in `Tessellate`.** The step is only bounded below by 1e-9; the piece count then overflows `int`, and on
  MSVC a fine step returns a single piece. NaN gets through too. The debug actor keeps the step ≥ 0.5°, so it doesn't
  happen yet.

### Tests (`Tests/CastleCore`)

- GoogleTest is downloaded without `URL_HASH`, so the archive's content isn't verified.

### Camera (`Source/Castle/CastleCameraPawn.*`)

- **No bounds.** WASD moves arbitrarily far from the castle; the pivot position isn't limited.
- **Fixed pivot height.** The pivot stays at the height where the pawn spawned (at the PlayerStart, somewhat above the
  ground) and rotates and zooms around it. With terrain it will need to snap to the ground by tracing down. The spring
  arm has collision disabled, so the camera could then dive into a hill.
- **Pause.** The camera runs on real time, so slowing down or speeding up the game doesn't affect it. In pause it stops,
  though. To work in pause it needs `bShouldPerformFullTickWhenPaused` on the controller (otherwise the camera manager
  doesn't update in pause), and `bTickEvenWhenPaused` on the pawn and spring arm (zoom easing). A full controller tick
  in pause lets all input through (`TickPlayerInput` gets `bGamePaused = false`), so `bTriggerWhenPaused` on actions no
  longer decides. Solve together with the design of pause: what should work while paused (building?).
- **Inconsistent smoothness.** Zoom eases in, panning and rotation start and stop instantly.
- **Input hard-coded.** Actions and the mapping context are created in C++; they can't be remapped or seen in the editor.
  With more actions, switch to `IA_`/`IMC_` assets. The build tool (`CastleWallBuildComponent`) creates its actions in
  code too, like the camera.
- **`MinZoom ≤ MaxZoom` isn't checked.** Both values are editable; swapped, `Clamp` behaves nonsensically.

### Build tool (`Source/Castle/CastleWallBuildComponent.*`, `Source/CastleCore/Public/CastleCore/WallPlacement.h`)

- **The preview copies the whole network** on every mouse move and everything gets redrawn. Fine for small networks;
  a large castle will need only the changed neighbourhood recomputed.
- **Snapping checks every node and wall** on every mouse move (`FindNearestNode`, `FindWallAt`). Thousands of elements
  will need a spatial index.
- **Cursor over the toolbar** still projects onto the ground, so the preview moves even under the toolbar.
- **Cursor and snap ring** are drawn with `DrawDebug*`, which doesn't exist in Shipping builds. The wall network is
  drawn with its own `ULineBatchComponent` and works everywhere.

### 3D walls (`Source/Castle/CastleWallMesh.*`)

- **No wall type in the model.** The core `Wall` doesn't know its type; the style (height, color) is one for the whole
  network. With more types, add the type to the model and decide what a node piece between different types looks like
  (height, material, features such as a palisade post or a higher crenellation section).
- **Everything is rebuilt.** On every change, and every frame of the preview, junctions, floor plan, limits and the
  whole mesh get recomputed, separately for the lines and the mesh. The target is castles with a few thousand
  elements, so recompute only the changed neighbourhood later, possibly a mesh per wall.
- **Faceted arcs.** Each 5° piece of an arc has its own flat normal. For a smooth arc, share normals along it.
- **No UVs or tangents.** Textures will need UVs (world-aligned, or along the wall).
- **No collision.** Walls don't block traces or navigation.
- **Stripes on walls (shadow acne).** A face the sun hits at a grazing angle shadows itself in stripes, which break at
  the boundary of the shadow map's resolution levels. It's a property of the shadows, not the geometry, so it shows
  only for some wall orientations. Check by rotating the sun; solve with the light's shadow bias or virtual shadow map
  settings once walls have a real material.

### Debug actor (`Source/Castle/CastleWallDebugActor.*`)

- An instance already placed keeps its saved `Nodes` and `Walls`. New default cases appear only on a new actor or after
  Reset to Default.

## Ideas

- **Build tool:** splitting walls the new wall crosses (X); snapping to angles (15°, 45°, 90°); parallels,
  perpendiculars and extensions as guide lines (`WallCurve::Project` serves lines too); deleting walls (picking with
  `WallNetwork::FindWallAt`). Done: placement with preview, chaining, snapping to nodes and to walls, which splits them
  (T).
- **Camera:** zoom towards the cursor instead of the screen center, tilting the camera towards horizontal when zooming
  in, mouse control (rotate by dragging with the middle button, pan by dragging or at the screen edge).
- **Undo/redo in the core:** `FScopedTransaction` in UE works only in the editor.
- **Corner style (sharp/beveled)** as a per-node switch in the UI.
- **Rooms** as faces of the planar wall graph. The player gives a room a function (living quarters, smithy, kitchen…)
  and upgrades; it is a dedicated space rather than a fixed building. Basis for the economy.
- **Round tower** from two half-circle arcs between the same nodes.
- **Wall types** (palisade, crenellated curtain wall, house wall, partition), openings (gates, windows), crenellations
  as instanced meshes. Later also early gunpowder-era fortifications (bastions, as at Kalich castle).
- **SVG output from the core tests** for visual checks without running UE.
- **Random junction tests:** generate random nodes and walls within the limits and check general properties, e.g. that
  no gap path runs into a wall and that the outline doesn't cross itself. Finds weaknesses systematically, not just the
  ones we think of.
- **Later:** terrain, verticality (floors, wall walks).
