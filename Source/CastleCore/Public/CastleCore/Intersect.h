#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"

#include <optional>

namespace Castle::Core
{
	/**
	 * Where the line through PointA along DirectionA crosses the line through PointB along DirectionB, anywhere along
	 * them. The directions are unit vectors; lines closer to parallel than AngleTolerance count as parallel and give empty.
	 */
	CASTLECORE_API std::optional<Vec2> IntersectLines(Vec2 PointA, Vec2 DirectionA, Vec2 PointB, Vec2 DirectionB);

	/**
	 * Where the ray from RayStart along the unit RayDirection leaves the circle, i.e. the farther of its two crossings.
	 * From a start inside the circle that is the only one ahead. Empty when the ray misses the circle or has left it
	 * already at RayStart.
	 */
	CASTLECORE_API std::optional<Vec2> IntersectRayWithCircle(Vec2 RayStart, Vec2 RayDirection, Vec2 Center, double Radius);
}
