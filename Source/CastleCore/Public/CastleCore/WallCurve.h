#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"

#include <optional>
#include <vector>

namespace Castle::Core
{
	/**
	 * A straight segment or a circular arc, parametrized by T from 0 (start) to 1 (end).
	 * Arcs are kept exact: the offset of an arc is a concentric arc, not an approximation.
	 */
	class WallCurve
	{
	public:
		enum class EType
		{
			Line,
			Arc,
		};

		CASTLECORE_API static WallCurve MakeLine(Vec2 Start, Vec2 End);

		/** Arc around InCenter starting at InStartAngle (radians). A positive InSweepAngle runs counterclockwise. */
		CASTLECORE_API static WallCurve MakeArc(Vec2 InCenter, double InRadius, double InStartAngle, double InSweepAngle);

		/**
		 * Curve from Start to End shaped by a bulge, as in DXF polylines: 0 is a straight line, otherwise
		 * Bulge = tan(theta / 4) where theta is the arc's included angle. A positive bulge runs counterclockwise,
		 * so the arc bows out to the right of Start->End. 1 is a half circle.
		 */
		CASTLECORE_API static WallCurve FromBulge(Vec2 Start, Vec2 End, double Bulge);

		EType GetType() const { return Type; }
		bool IsArc() const { return Type == EType::Arc; }

		CASTLECORE_API Vec2 GetStart() const;
		CASTLECORE_API Vec2 GetEnd() const;
		CASTLECORE_API Vec2 PointAt(double T) const;

		/** Unit tangent in the direction of travel. Zero for a degenerate line. */
		CASTLECORE_API Vec2 TangentAt(double T) const;

		CASTLECORE_API double GetLength() const;

		/**
		 * Parameter of the point on the curve nearest to Point, where a perpendicular from Point meets the curve.
		 * Empty when that point would lie beyond the ends, or when the curve gives no direction to measure from
		 * (a line of no length, the center of an arc).
		 */
		CASTLECORE_API std::optional<double> Project(Vec2 Point) const;

		/** Arc only. */
		Vec2 GetCenter() const { return Center; }
		double GetRadius() const { return Radius; }
		double GetStartAngle() const { return StartAngle; }
		double GetSweepAngle() const { return SweepAngle; }

		/**
		 * Parallel curve at OffsetDistance to the left of the direction of travel (negative = to the right).
		 * Empty when an arc collapses, i.e. OffsetDistance reaches its radius on the inner side.
		 */
		CASTLECORE_API std::optional<WallCurve> Offset(double OffsetDistance) const;

		/** The part of the curve between parameters T0 and T1, running in the same direction when T0 < T1. */
		CASTLECORE_API WallCurve Segment(double T0, double T1) const;

		/**
		 * Points along the curve, both ends included. Arcs are split so that no piece turns by more
		 * than MaxAngleStep (radians); a line yields just its two ends.
		 */
		CASTLECORE_API std::vector<Vec2> Tessellate(double MaxAngleStep) const;

	private:
		WallCurve() = default;

		EType Type = EType::Line;

		// Line
		Vec2 LineStart;
		Vec2 LineEnd;

		// Arc
		Vec2 Center;
		double Radius = 0.0;
		double StartAngle = 0.0;
		double SweepAngle = 0.0;
	};
}
