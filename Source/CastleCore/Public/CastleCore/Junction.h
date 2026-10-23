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

	enum class EGapShape
	{
		/** Not resolved yet: arcs, or the free end of a single wall. */
		Unresolved,

		/** The sides meet in a single point: they cross, or continue each other for walls in line. */
		Corner,

		/**
		 * The wider wall's corner sticks out past the thinner wall's side (a much thinner wall at an angle around
		 * 180 degrees). The wider wall's side continues past the node in a straight line to the thinner wall's side.
		 */
		StraightExtension,

		/**
		 * As above, but the straight line would be too long, or never meet the thinner side, so a line of the
		 * maximum extension length joins the wider wall's corner to the thinner wall's side.
		 */
		BeveledExtension,

		/** The extension ends at the middle of the thinner wall's side, the farthest it may reach along a short wall. */
		ExtensionToSideMiddle,

		/** Outer side of a bend with a sharp corner: both sides are extended until they meet. */
		SharpOuterCorner,

		/** Outer side of a bend with a beveled corner: the ends of both sides are joined directly. */
		BeveledOuterCorner,
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

		EGapShape Shape = EGapShape::Unresolved;

		/**
		 * How the outline gets from one side facing the gap to the other: the first wall's side runs up to
		 * Path.front(), straight lines join consecutive points, and the second wall's side continues from
		 * Path.back(). A single point where the sides simply meet. Empty while Unresolved.
		 */
		std::vector<Vec2> Path;
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
