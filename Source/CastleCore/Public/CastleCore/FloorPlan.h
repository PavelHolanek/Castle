#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Junction.h"
#include "CastleCore/Polygon.h"
#include "CastleCore/Vec2.h"
#include "CastleCore/WallCurve.h"
#include "CastleCore/WallNetwork.h"

#include <optional>
#include <vector>

namespace Castle::Core
{
	struct FloorPlanSettings
	{
		JunctionSettings Junctions;

		/**
		 * Nodes whose joints are left unresolved: their walls end in plain caps and they get no piece.
		 * The game passes nodes outside the limits, where the joint would be meaningless.
		 */
		std::vector<NodeId> SkippedNodes;
	};

	/**
	 * What is left of a wall once the joints at both ends are cut off. Each cut runs across the wall,
	 * perpendicular to its reference line.
	 */
	struct WallBody
	{
		WallId Wall;

		/** Where the wall is cut off, measured along the reference line from NodeA and from NodeB. */
		double StartCut = 0.0;
		double EndCut = 0.0;

		/**
		 * The joints at both ends take up the whole wall, so nothing is left of it and the two node pieces
		 * may overlap. Not prevented yet, see Docs/Backlog.md.
		 */
		bool bConsumedByJoints = false;

		/**
		 * Both sides between the cuts, running from NodeA's end to NodeB's. Empty when the joints consume
		 * the wall or a side of an arc collapses.
		 */
		std::optional<WallCurve> Left;
		std::optional<WallCurve> Right;
	};

	/** The space between the cuts of the walls meeting at a node, owned by the node rather than any wall. */
	struct NodePiece
	{
		NodeId Node;

		/** Counterclockwise outline, without repeated or collinear points. */
		std::vector<Vec2> Polygon;
	};

	struct FloorPlan
	{
		/** One for each wall of the network, in the order of WallNetwork::GetWallIds. */
		std::vector<WallBody> Walls;

		/** Only nodes whose joint leaves some space between the cuts get a piece; walls in line, for example, don't. */
		std::vector<NodePiece> Nodes;

		/** Nodes where walls meet but the joint wasn't resolved (arcs, skipped nodes). Their walls end in plain caps. */
		std::vector<NodeId> UnresolvedNodes;
	};

	/**
	 * Splits the network's floor area into wall bodies and node pieces. At a resolved node each wall is cut where its
	 * joint first touches one of its sides, coming from the far end of the wall: across the point where the joint path
	 * meets the side farther from the node. Everything between the cuts belongs to the node.
	 * A free end is cut at the node itself, which leaves a plain perpendicular cap.
	 */
	CASTLECORE_API FloorPlan BuildFloorPlan(const WallNetwork& Network, const FloorPlanSettings& Settings = {});

	/** Counterclockwise outline of the body, arcs split as in WallCurve::Tessellate. Empty when the body has no sides. */
	CASTLECORE_API std::vector<Vec2> GetBodyPolygon(const WallBody& Body, double MaxAngleStep);
}
