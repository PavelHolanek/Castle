#pragma once

#include "CoreMinimal.h"
#include "CastleWallViewMode.h"
#include "CastleCore/FloorPlan.h"
#include "CastleCore/Junction.h"
#include "CastleCore/WallLimits.h"
#include "CastleCore/WallNetwork.h"

class ULineBatchComponent;

struct FCastleWallDrawOptions
{
	/** Maximum angle between two neighbouring points of a drawn arc, in degrees. */
	double ArcStep = 5.0;

	float LineThickness = 3.f;
	float NodeSize = 12.f;

	/** Height above the transform's origin at which the lines are drawn. */
	double DrawHeight = 5.0;

	ECastleWallViewMode ViewMode = ECastleWallViewMode::FloorPlan;

	Castle::Core::JunctionSettings Junctions;
	Castle::Core::WallLimits Limits;

	/** Number a wall goes by in the report. Its id when not set. */
	TFunction<int32(Castle::Core::WallId)> WallLabel;

	/** Walls shown in CastleWallDrawing::HighlightColor, by id value, e.g. the one the demolish tool points at. */
	TSet<uint32> HighlightedWalls;
};

/** Nodes and walls outside the limits, by id value. */
struct FCastleWallFaults
{
	TSet<uint32> Nodes;
	TSet<uint32> Walls;
};

/** What the drawing found, in readable form. Nodes go by their id, positions are relative to the transform. */
struct FCastleWallDrawReport
{
	TArray<FString> LimitViolations;
	TArray<FString> JunctionCorners;
	TArray<FString> FloorPlanPieces;
};

namespace CastleWallDrawing
{
	/** Color of highlighted walls, in lines as well as in 3D. */
	const FLinearColor HighlightColor(1.f, 0.8f, 0.f);

	/**
	 * Replaces whatever Lines drew before with the network, positions taken relative to Transform.
	 *
	 * The floor plan shows each wall's reference line (black), its body between the cuts (red sides, blue cuts),
	 * the nodes (green) and the node pieces (orange). Walls and nodes outside the limits are magenta; a node outside
	 * the limits is left unresolved, so its walls end in plain caps. The raw view shows full-length sides with
	 * perpendicular caps and the junction paths (orange) instead. Highlighted walls get their full outline over that,
	 * thicker. With 3D walls, which CastleWallMesh builds, only what is outside the limits gets drawn, magenta.
	 */
	void DrawNetwork(ULineBatchComponent& Lines, const Castle::Core::WallNetwork& Network, const FTransform& Transform,
		const FCastleWallDrawOptions& Options, FCastleWallDrawReport* Report = nullptr);

	FCastleWallFaults FindFaults(const Castle::Core::WallNetwork& Network, const Castle::Core::WallLimits& Limits);
}
