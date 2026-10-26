#include "CastleCore/Intersect.h"

#include <cmath>

namespace Castle::Core
{
	std::optional<Vec2> IntersectLines(Vec2 PointA, Vec2 DirectionA, Vec2 PointB, Vec2 DirectionB)
	{
		const double Denominator = Cross(DirectionA, DirectionB);
		if (std::abs(Denominator) <= AngleTolerance)
		{
			return std::nullopt;
		}
		return PointA + DirectionA * (Cross(PointB - PointA, DirectionB) / Denominator);
	}

	std::optional<Vec2> IntersectRayWithCircle(Vec2 RayStart, Vec2 RayDirection, Vec2 Center, double Radius)
	{
		const Vec2 FromCenter = RayStart - Center;
		const double Projection = Dot(FromCenter, RayDirection);
		const double Discriminant = Projection * Projection - Dot(FromCenter, FromCenter) + Radius * Radius;
		if (Discriminant < 0.0)
		{
			return std::nullopt;
		}

		const double Along = -Projection + std::sqrt(Discriminant);
		if (Along < 0.0)
		{
			return std::nullopt;
		}
		return RayStart + RayDirection * Along;
	}
}
