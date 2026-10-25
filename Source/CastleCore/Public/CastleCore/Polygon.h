#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"

#include <vector>

namespace Castle::Core
{
	/** Positive for a counterclockwise polygon. */
	CASTLECORE_API double GetSignedArea(const std::vector<Vec2>& Polygon);

	/** Counterclockwise polygon with Segments corners on the circle, the first one at angle 0. */
	CASTLECORE_API std::vector<Vec2> MakeRegularPolygon(Vec2 Center, double Radius, int Segments);

	/**
	 * The part of Subject inside the convex, counterclockwise polygon Clip (Sutherland-Hodgman). Empty when they don't
	 * overlap. A concave Subject cut into several parts comes out as one outline joined along Clip's edges.
	 */
	CASTLECORE_API std::vector<Vec2> ClipToConvex(const std::vector<Vec2>& Subject, const std::vector<Vec2>& Clip);
}
