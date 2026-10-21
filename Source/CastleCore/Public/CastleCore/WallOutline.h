#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/WallCurve.h"
#include "CastleCore/WallNetwork.h"

#include <optional>

namespace Castle::Core
{
	/**
	 * Outline of a single wall before junctions are resolved: both sides run the full length of the
	 * reference line and each end is closed by a straight cap perpendicular to it.
	 */
	struct RawWallOutline
	{
		WallCurve Reference;

		/** Empty when the side of an arc collapses (its width reaches the radius on the inner side). */
		std::optional<WallCurve> Left;
		std::optional<WallCurve> Right;

		/** Cap at NodeA, from the right side to the left side. */
		WallCurve StartCap;

		/** Cap at NodeB, from the right side to the left side. */
		WallCurve EndCap;
	};

	/** Empty for an unknown wall. */
	CASTLECORE_API std::optional<RawWallOutline> BuildRawOutline(const WallNetwork& Network, WallId Id);
}
