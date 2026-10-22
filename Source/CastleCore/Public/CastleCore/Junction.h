#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"
#include "CastleCore/WallNetwork.h"

#include <optional>
#include <vector>

namespace Castle::Core
{
	struct JunctionSettings
	{
		/** Longest a side extension may get, as a multiple of the wider wall's width on that side. */
		double MaxExtensionFactor = 2.0;
	};

	/** A wall attached to a junction node, seen from the node. */
	struct JunctionWall
	{
		WallId Id;

		/** Unit tangent of the reference line at the node, pointing away from it. */
		Vec2 Direction;

		/** Angle of Direction, counterclockwise from +X, in [0, 2 Pi). */
		double DirectionAngle = 0.0;

		/**
		 * Widths on the counterclockwise (left of Direction) and clockwise (right of Direction) side.
		 * For a wall that ends at the node these are its WidthRight and WidthLeft, since it runs towards the node.
		 */
		double WidthCounterclockwise = 0.0;
		double WidthClockwise = 0.0;

		/** Length of the reference line. */
		double Length = 0.0;

		bool bIsArc = false;
	};

	enum class EExtensionShape
	{
		/** Continues the wider wall's side in a straight line to where it meets the thinner wall's side. */
		Straight,

		/** The straight line would be too long (or never meet, for walls in line), so it turns towards the
		 * thinner wall's side and reaches it at the maximum length. */
		Beveled,

		/** Ends at the middle of the thinner wall's side, the farthest it may reach along a short wall. */
		ToSideMiddle,
	};

	/**
	 * Where a much thinner wall's side passes the wider wall's side behind the node (an obtuse angle, or walls
	 * in line), the wider wall's side is continued past the node to the thinner wall's side, smoothing out the step.
	 */
	struct SideExtension
	{
		/** Index into Junction::Walls of the wider wall, whose side is extended. */
		size_t WallIndex = 0;

		/** Where the wider wall's raw side ends at the node. The extension runs from here to the gap's Corner. */
		Vec2 Start;

		EExtensionShape Shape = EExtensionShape::Straight;
	};

	/**
	 * The space between two neighbouring walls at a node, going counterclockwise from the first wall to the
	 * second. The sides facing the gap are the first wall's counterclockwise side and the second wall's
	 * clockwise side.
	 */
	struct JunctionGap
	{
		/** Angle swept from the first wall to the second, in [0, 2 Pi]. 2 Pi when only one wall meets at the node. */
		double Angle = 0.0;

		/**
		 * Where the outline passes from one side facing the gap to the other: where the sides intersect, or
		 * where an Extension reaches the thinner wall's side.
		 * So far computed only for straight walls meeting at up to 180 degrees.
		 */
		std::optional<Vec2> Corner;

		std::optional<SideExtension> Extension;
	};

	struct Junction
	{
		NodeId Node;
		Vec2 Position;

		/** Sorted counterclockwise by DirectionAngle. */
		std::vector<JunctionWall> Walls;

		/** Gaps[i] lies between Walls[i] and Walls[(i + 1) % Walls.size()]. */
		std::vector<JunctionGap> Gaps;
	};

	/** Empty for an unknown node. */
	CASTLECORE_API std::optional<Junction> BuildJunction(const WallNetwork& Network, NodeId Id, const JunctionSettings& Settings = {});
}
