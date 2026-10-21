#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"
#include "CastleCore/WallCurve.h"

#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Castle::Core
{
	/** Stable handle to an element of a WallNetwork. Ids are never reused within one network. */
	template <typename TTag>
	struct ElementId
	{
		static constexpr uint32_t InvalidValue = UINT32_MAX;

		uint32_t Value = InvalidValue;

		constexpr bool IsValid() const { return Value != InvalidValue; }

		constexpr auto operator<=>(const ElementId&) const = default;
	};

	using NodeId = ElementId<struct NodeTag>;
	using WallId = ElementId<struct WallTag>;

	struct Node
	{
		Vec2 Position;

		/** Walls that start or end here, in the order they were attached. */
		std::vector<WallId> Walls;
	};

	/**
	 * A wall between two nodes. The reference line runs from NodeA to NodeB; the wall extends
	 * WidthLeft to its left and WidthRight to its right. Equal widths put the reference line in the middle.
	 */
	struct Wall
	{
		NodeId NodeA;
		NodeId NodeB;
		double WidthLeft = 0.0;
		double WidthRight = 0.0;

		/** Shape of the reference line, see WallCurve::FromBulge. 0 is straight. */
		double Bulge = 0.0;
	};

	/**
	 * Walls connected through shared nodes. This is the authoritative model of what the player built;
	 * outlines and junctions are derived from it.
	 *
	 * Two walls may connect the same pair of nodes (e.g. two half arcs forming a round tower),
	 * but a wall cannot start and end at the same node.
	 */
	class WallNetwork
	{
	public:
		CASTLECORE_API NodeId AddNode(Vec2 Position);

		/** Invalid id when a node doesn't exist, both ends are the same node, or a width is negative. */
		CASTLECORE_API WallId AddWall(NodeId NodeA, NodeId NodeB, double WidthLeft, double WidthRight, double Bulge = 0.0);

		CASTLECORE_API bool MoveNode(NodeId Id, Vec2 Position);
		CASTLECORE_API bool RemoveWall(WallId Id);

		/** Removes the node together with every wall attached to it. */
		CASTLECORE_API bool RemoveNode(NodeId Id);

		CASTLECORE_API const Node* FindNode(NodeId Id) const;
		CASTLECORE_API const Wall* FindWall(WallId Id) const;

		/** Walls that start or end at the node. Empty for an unknown node. */
		CASTLECORE_API std::span<const WallId> GetNodeWalls(NodeId Id) const;

		/** Reference line from NodeA to NodeB. Empty for an unknown wall. */
		CASTLECORE_API std::optional<WallCurve> GetReferenceCurve(WallId Id) const;

		CASTLECORE_API std::vector<NodeId> GetNodeIds() const;
		CASTLECORE_API std::vector<WallId> GetWallIds() const;

		size_t GetNodeCount() const { return NodeCount; }
		size_t GetWallCount() const { return WallCount; }

	private:
		Node* FindNodeMutable(NodeId Id);

		// Indexed by id; removed elements leave an empty slot so that ids stay stable.
		std::vector<std::optional<Node>> Nodes;
		std::vector<std::optional<Wall>> Walls;
		size_t NodeCount = 0;
		size_t WallCount = 0;
	};
}
