#include "CastleCore/WallLimits.h"

#include <algorithm>
#include <cmath>

namespace Castle::Core
{
	namespace
	{
		// A value exactly at a limit passes, even when rounding nudges it just past.
		bool IsBelow(double Value, double Min, double Tolerance)
		{
			return Value < Min - Tolerance;
		}

		bool IsAbove(double Value, double Max, double Tolerance)
		{
			return Value > Max + Tolerance;
		}
	}

	const char* ToString(ELimitViolation Violation)
	{
		switch (Violation)
		{
		case ELimitViolation::TooShort: return "TooShort";
		case ELimitViolation::TooLong: return "TooLong";
		case ELimitViolation::TooThin: return "TooThin";
		case ELimitViolation::TooThick: return "TooThick";
		case ELimitViolation::ArcRadiusTooSmall: return "ArcRadiusTooSmall";
		case ELimitViolation::ArcRadiusTooLarge: return "ArcRadiusTooLarge";
		case ELimitViolation::ArcAngleTooSmall: return "ArcAngleTooSmall";
		case ELimitViolation::ArcAngleTooLarge: return "ArcAngleTooLarge";
		case ELimitViolation::SideCollapses: return "SideCollapses";
		case ELimitViolation::JointTooSharp: return "JointTooSharp";
		}
		return "Unknown";
	}

	std::vector<ELimitViolation> CheckWall(const WallNetwork& Network, WallId Id, const WallLimits& Limits)
	{
		std::vector<ELimitViolation> Violations;
		const Wall* Found = Network.FindWall(Id);
		if (!Found)
		{
			return Violations;
		}

		const WallCurve Reference = *Network.GetReferenceCurve(Id);
		const double WallLength = Reference.GetLength();
		if (IsBelow(WallLength, Limits.MinLength, DistanceTolerance))
		{
			Violations.push_back(ELimitViolation::TooShort);
		}
		if (IsAbove(WallLength, Limits.MaxLength, DistanceTolerance))
		{
			Violations.push_back(ELimitViolation::TooLong);
		}

		const double TotalWidth = Found->WidthLeft + Found->WidthRight;
		if (IsBelow(TotalWidth, Limits.MinWidth, DistanceTolerance))
		{
			Violations.push_back(ELimitViolation::TooThin);
		}
		if (IsAbove(TotalWidth, Limits.MaxWidth, DistanceTolerance))
		{
			Violations.push_back(ELimitViolation::TooThick);
		}

		if (Reference.IsArc())
		{
			if (IsBelow(Reference.GetRadius(), Limits.MinArcRadius, DistanceTolerance))
			{
				Violations.push_back(ELimitViolation::ArcRadiusTooSmall);
			}
			if (IsAbove(Reference.GetRadius(), Limits.MaxArcRadius, DistanceTolerance))
			{
				Violations.push_back(ELimitViolation::ArcRadiusTooLarge);
			}

			const double ArcAngle = std::abs(Reference.GetSweepAngle());
			if (IsBelow(ArcAngle, Limits.MinArcAngle, AngleTolerance))
			{
				Violations.push_back(ELimitViolation::ArcAngleTooSmall);
			}
			if (IsAbove(ArcAngle, Limits.MaxArcAngle, AngleTolerance))
			{
				Violations.push_back(ELimitViolation::ArcAngleTooLarge);
			}

			if (!Reference.Offset(Found->WidthLeft) || !Reference.Offset(-Found->WidthRight))
			{
				Violations.push_back(ELimitViolation::SideCollapses);
			}
		}

		return Violations;
	}

	std::vector<ELimitViolation> CheckNode(const WallNetwork& Network, NodeId Id, const WallLimits& Limits)
	{
		std::vector<ELimitViolation> Violations;
		const std::optional<double> SmallestAngle = GetSmallestJointAngle(Network, Id);
		if (SmallestAngle && IsBelow(*SmallestAngle, Limits.MinJointAngle, AngleTolerance))
		{
			Violations.push_back(ELimitViolation::JointTooSharp);
		}
		return Violations;
	}

	std::optional<double> GetSmallestJointAngle(const WallNetwork& Network, NodeId Id)
	{
		const std::vector<WallEnd> Walls = Network.GetWallsAround(Id);
		if (Walls.size() < 2)
		{
			return std::nullopt;
		}

		// The angle between any two walls spans one or more gaps between neighbours, so the smallest angle is the smallest
		// gap, at most Pi as the gaps add up to a full turn. The last gap wraps around past +X.
		double Smallest = Walls.front().DirectionAngle + 2.0 * Pi - Walls.back().DirectionAngle;
		for (size_t Index = 1; Index < Walls.size(); ++Index)
		{
			Smallest = std::min(Smallest, Walls[Index].DirectionAngle - Walls[Index - 1].DirectionAngle);
		}
		return Smallest;
	}
}
