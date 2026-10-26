#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/Vec2.h"
#include "CastleCore/WallLimits.h"
#include "CastleCore/WallNetwork.h"

#include <optional>
#include <vector>

namespace Castle::Core
{
	/**
	 * Places straight walls in a chain: the first click picks the start, every further click the end of a wall,
	 * which then becomes the start of the next one. The chain ends with Cancel, or when a wall ends at an existing
	 * node or on an existing wall. Between clicks the cursor drives a preview, the network with the wall added, so that
	 * its junctions and limits show before anything is committed. A wall outside the limits can't be committed.
	 *
	 * Near the cursor, an existing node is used instead of a new one, for the start as well as the end. How near depends
	 * on the node: SnapRadius, or more around wide walls (see GetSnapRadius). Otherwise, with the cursor over an existing
	 * wall (see WallNetwork::FindWallAt), a new node on its reference line splits it, and its parts have to keep within
	 * the limits too.
	 */
	class WallPlacement
	{
	public:
		enum class EState
		{
			PickingStart,
			PickingEnd,
		};

		enum class EClickResult
		{
			/** The click picked the start; the end follows the cursor now. */
			Started,

			/** The wall was added to the network. Its end starts the next wall, unless it joined an existing node or wall. */
			Committed,

			/** The wall can't be placed there; nothing changed. */
			Rejected,
		};

		/** Widths of the walls to place, to the left and right of the line from start to end. */
		CASTLECORE_API void SetWidths(double InWidthLeft, double InWidthRight);

		void SetLimits(const WallLimits& InLimits) { Limits = InLimits; }

		/** How far the cursor snaps to a node, as a multiple of the total width of the widest wall there. */
		void SetSnapWidthFactor(double InSnapWidthFactor) { SnapWidthFactor = InSnapWidthFactor; }

		/** How close the cursor has to be to snap to Node: SnapRadius, or farther around a wide wall. */
		CASTLECORE_API double GetSnapRadius(const WallNetwork& Network, NodeId Node, double SnapRadius) const;

		/** The cursor moved. While picking the end, rebuilds the preview from Network. */
		CASTLECORE_API void Hover(const WallNetwork& Network, Vec2 Cursor, double SnapRadius = 0.0);

		CASTLECORE_API EClickResult Click(WallNetwork& Network, Vec2 Cursor, double SnapRadius = 0.0);

		/** Drops the wall being placed and ends the chain. */
		CASTLECORE_API void Cancel();

		EState GetState() const { return State; }

		/** The existing node the last Hover or Click snapped to. Invalid when it didn't snap to a node. */
		NodeId GetSnapNode() const { return SnapNode; }

		/** Where the last Hover or Click would split an existing wall. Empty when it didn't snap to a wall. */
		const std::optional<WallPoint>& GetSnapWall() const { return SnapWall; }

		/** The network with the wall being placed. Empty while picking the start. */
		const std::optional<WallNetwork>& GetPreview() const { return Preview; }

		/** The wall being placed, in the preview. Invalid when it can't be placed at all. */
		WallId GetPreviewWall() const { return PreviewWall; }

		/** Limits the previewed wall breaks, on its own or where it meets other walls. */
		const std::vector<ELimitViolation>& GetPreviewViolations() const { return PreviewViolations; }

		/** Whether a click would commit the previewed wall. */
		bool CanCommit() const { return PreviewWall.IsValid() && PreviewViolations.empty(); }

	private:
		/** A node to be: an existing one, a new one splitting an existing wall, or a new one on its own at Position. */
		struct Endpoint
		{
			Vec2 Position;
			NodeId Node;

			/** The wall to split at Position, and where along it. */
			WallId Wall;
			double WallT = 0.0;
		};

		/**
		 * Where the cursor points: the nearest node within its snap radius other than Excluded, else a point on the wall
		 * under the cursor, else the cursor itself.
		 */
		Endpoint Snap(const WallNetwork& Network, Vec2 Cursor, double SnapRadius, NodeId Excluded = {});

		/** Makes Point a node of Network: the existing one, or a new one splitting a wall or on its own. */
		static NodeId MakeNode(WallNetwork& Network, const Endpoint& Point);

		void UpdatePreview(const WallNetwork& Network, Vec2 Cursor, double SnapRadius);

		EState State = EState::PickingStart;
		Endpoint Start;
		double WidthLeft = 0.0;
		double WidthRight = 0.0;
		WallLimits Limits;
		double SnapWidthFactor = 0.75;
		NodeId SnapNode;
		std::optional<WallPoint> SnapWall;

		std::optional<WallNetwork> Preview;
		WallId PreviewWall;
		NodeId PreviewEndNode;

		/** The wall being placed ends at an existing node or on an existing wall, which ends the chain. */
		bool bPreviewEndJoins = false;
		std::vector<ELimitViolation> PreviewViolations;
	};
}
