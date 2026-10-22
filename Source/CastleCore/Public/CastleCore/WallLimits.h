#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"
#include "CastleCore/WallNetwork.h"

#include <optional>
#include <vector>

namespace Castle::Core
{
	/**
	 * Ranges the walls must stay within. The network itself accepts anything; the build tool checks
	 * these before it commits a change. Lengths are in cm, angles in radians.
	 */
	struct WallLimits
	{
		/** Length of the reference line (along the arc for arcs). */
		double MinLength = 100.0;
		double MaxLength = 5000.0;

		/** Total width, WidthLeft + WidthRight. */
		double MinWidth = 40.0;
		double MaxWidth = 500.0;

		/** Radius of an arc's reference line. */
		double MinArcRadius = 300.0;
		double MaxArcRadius = 10000.0;

		/** Included angle of an arc. A full circle (round tower) takes at least two walls. */
		double MinArcAngle = DegreesToRadians(5.0);
		double MaxArcAngle = DegreesToRadians(180.0);

		/** Smallest angle between two walls meeting at a node. */
		double MinJointAngle = DegreesToRadians(30.0);
	};

	enum class ELimitViolation
	{
		TooShort,
		TooLong,
		TooThin,
		TooThick,
		ArcRadiusTooSmall,
		ArcRadiusTooLarge,
		ArcAngleTooSmall,
		ArcAngleTooLarge,
		/** The width on the inner side of an arc reaches its center. */
		SideCollapses,
		JointTooSharp,
	};

	CASTLECORE_API const char* ToString(ELimitViolation Violation);

	/** Limits the wall breaks on its own. Empty when it is within all of them or doesn't exist. */
	CASTLECORE_API std::vector<ELimitViolation> CheckWall(const WallNetwork& Network, WallId Id, const WallLimits& Limits);

	/** Limits broken where walls meet at the node. Empty when there are none or the node doesn't exist. */
	CASTLECORE_API std::vector<ELimitViolation> CheckNode(const WallNetwork& Network, NodeId Id, const WallLimits& Limits);

	/**
	 * Smallest angle (radians, 0 to Pi) between two walls meeting at the node, measured between their
	 * tangents there. Empty when fewer than two walls meet at the node.
	 */
	CASTLECORE_API std::optional<double> GetSmallestJointAngle(const WallNetwork& Network, NodeId Id);
}
