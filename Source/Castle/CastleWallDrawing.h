#pragma once

#include "CoreMinimal.h"
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

	Castle::Core::JunctionSettings Junctions;
	Castle::Core::WallLimits Limits;

	/** Number a wall goes by in the report. Its id when not set. */
	TFunction<int32(Castle::Core::WallId)> WallLabel;
};

/** What the drawing found, in readable form. Nodes go by their id. */
struct FCastleWallDrawReport
{
	TArray<FString> LimitViolations;
	TArray<FString> JunctionCorners;
};

namespace CastleWallDrawing
{
	/**
	 * Replaces whatever Lines drew before with the network: each wall's reference line (black), raw sides (red)
	 * and end caps (blue), the nodes (green) and the junction paths (orange). Walls and nodes outside the limits
	 * are magenta; a node outside the limits gets no junction paths. Network positions are taken relative to Transform.
	 */
	void DrawNetwork(ULineBatchComponent& Lines, const Castle::Core::WallNetwork& Network, const FTransform& Transform,
		const FCastleWallDrawOptions& Options, FCastleWallDrawReport* Report = nullptr);
}
