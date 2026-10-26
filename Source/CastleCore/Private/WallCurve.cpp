#include "CastleCore/WallCurve.h"

#include <algorithm>
#include <cmath>

namespace Castle::Core
{
	WallCurve WallCurve::MakeLine(Vec2 Start, Vec2 End)
	{
		WallCurve Curve;
		Curve.Type = EType::Line;
		Curve.LineStart = Start;
		Curve.LineEnd = End;
		return Curve;
	}

	WallCurve WallCurve::MakeArc(Vec2 InCenter, double InRadius, double InStartAngle, double InSweepAngle)
	{
		WallCurve Curve;
		Curve.Type = EType::Arc;
		Curve.Center = InCenter;
		Curve.Radius = InRadius;
		Curve.StartAngle = InStartAngle;
		Curve.SweepAngle = InSweepAngle;
		return Curve;
	}

	WallCurve WallCurve::FromBulge(Vec2 Start, Vec2 End, double Bulge)
	{
		const Vec2 Chord = End - Start;
		const double HalfChord = 0.5 * Length(Chord);
		if (std::abs(Bulge) <= AngleTolerance || HalfChord <= DistanceTolerance)
		{
			return MakeLine(Start, End);
		}

		// Signed distance of the center from the chord midpoint, measured to the left of the chord.
		// Follows from R = HalfChord * (1 + Bulge^2) / (2 * |Bulge|) and the sagitta Bulge * HalfChord.
		const double CenterOffset = HalfChord * (1.0 - Bulge * Bulge) / (2.0 * Bulge);
		const Vec2 Midpoint = Start + Chord * 0.5;
		const Vec2 ArcCenter = Midpoint + PerpLeft(Chord / (2.0 * HalfChord)) * CenterOffset;

		const double ArcRadius = HalfChord * (1.0 + Bulge * Bulge) / (2.0 * std::abs(Bulge));
		const Vec2 ToStart = Start - ArcCenter;
		return MakeArc(ArcCenter, ArcRadius, std::atan2(ToStart.Y, ToStart.X), 4.0 * std::atan(Bulge));
	}

	Vec2 WallCurve::GetStart() const
	{
		return PointAt(0.0);
	}

	Vec2 WallCurve::GetEnd() const
	{
		return PointAt(1.0);
	}

	Vec2 WallCurve::PointAt(double T) const
	{
		if (Type == EType::Line)
		{
			return LineStart + (LineEnd - LineStart) * T;
		}
		return PointOnCircle(Center, Radius, StartAngle + SweepAngle * T);
	}

	Vec2 WallCurve::TangentAt(double T) const
	{
		if (Type == EType::Line)
		{
			return Normalized(LineEnd - LineStart);
		}
		const double Angle = StartAngle + SweepAngle * T;
		const Vec2 Radial{std::cos(Angle), std::sin(Angle)};
		return SweepAngle >= 0.0 ? PerpLeft(Radial) : -PerpLeft(Radial);
	}

	double WallCurve::GetLength() const
	{
		if (Type == EType::Line)
		{
			return Distance(LineStart, LineEnd);
		}
		return Radius * std::abs(SweepAngle);
	}

	std::optional<double> WallCurve::Project(Vec2 Point) const
	{
		double T = 0.0;
		if (Type == EType::Line)
		{
			const Vec2 Direction = LineEnd - LineStart;
			const double LengthSquared = Dot(Direction, Direction);
			if (LengthSquared <= DistanceTolerance * DistanceTolerance)
			{
				return std::nullopt;
			}
			T = Dot(Point - LineStart, Direction) / LengthSquared;
		}
		else
		{
			const Vec2 FromCenter = Point - Center;
			if (Length(FromCenter) <= DistanceTolerance || std::abs(SweepAngle) <= AngleTolerance)
			{
				return std::nullopt;
			}

			// Measured from the middle of the arc, so that the angle doesn't wrap around anywhere on it.
			const double MiddleAngle = StartAngle + 0.5 * SweepAngle;
			const double FromMiddle = std::remainder(std::atan2(FromCenter.Y, FromCenter.X) - MiddleAngle, 2.0 * Pi);
			T = 0.5 + FromMiddle / SweepAngle;
		}

		// A point at an end still counts when rounding moves it just past.
		if (T < -AngleTolerance || T > 1.0 + AngleTolerance)
		{
			return std::nullopt;
		}
		return std::clamp(T, 0.0, 1.0);
	}

	std::optional<WallCurve> WallCurve::Offset(double OffsetDistance) const
	{
		if (Type == EType::Line)
		{
			const Vec2 Shift = PerpLeft(Normalized(LineEnd - LineStart)) * OffsetDistance;
			return MakeLine(LineStart + Shift, LineEnd + Shift);
		}

		// The center lies to the left of a counterclockwise arc, so moving left shrinks the radius.
		const double OffsetRadius = SweepAngle >= 0.0 ? Radius - OffsetDistance : Radius + OffsetDistance;
		if (OffsetRadius <= DistanceTolerance)
		{
			return std::nullopt;
		}
		return MakeArc(Center, OffsetRadius, StartAngle, SweepAngle);
	}

	WallCurve WallCurve::Segment(double T0, double T1) const
	{
		if (Type == EType::Line)
		{
			return MakeLine(PointAt(T0), PointAt(T1));
		}
		return MakeArc(Center, Radius, StartAngle + SweepAngle * T0, SweepAngle * (T1 - T0));
	}

	std::vector<Vec2> WallCurve::Tessellate(double MaxAngleStep) const
	{
		if (Type == EType::Line)
		{
			return {LineStart, LineEnd};
		}

		const double Step = std::max(MaxAngleStep, AngleTolerance);
		const int PieceCount = std::max(1, static_cast<int>(std::ceil(std::abs(SweepAngle) / Step)));

		std::vector<Vec2> Points;
		Points.reserve(static_cast<size_t>(PieceCount) + 1);
		for (int Index = 0; Index <= PieceCount; ++Index)
		{
			Points.push_back(PointAt(static_cast<double>(Index) / PieceCount));
		}
		return Points;
	}
}
