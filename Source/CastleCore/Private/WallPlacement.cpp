#include "CastleCore/WallPlacement.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace Castle::Core
{
	void WallPlacement::SetWidths(double InWidthLeft, double InWidthRight)
	{
		WidthLeft = InWidthLeft;
		WidthRight = InWidthRight;
	}

	void WallPlacement::Hover(const WallNetwork& Network, Vec2 Cursor, double SnapRadius)
	{
		if (State == EState::PickingStart)
		{
			Snap(Network, Cursor, SnapRadius);
		}
		else
		{
			UpdatePreview(Network, Cursor, SnapRadius);
		}
	}

	WallPlacement::EClickResult WallPlacement::Click(WallNetwork& Network, Vec2 Cursor, double SnapRadius)
	{
		if (State == EState::PickingStart)
		{
			Start = Snap(Network, Cursor, SnapRadius);
			State = EState::PickingEnd;
			UpdatePreview(Network, Cursor, SnapRadius);
			return EClickResult::Started;
		}

		UpdatePreview(Network, Cursor, SnapRadius);
		if (!CanCommit())
		{
			return EClickResult::Rejected;
		}

		// The preview is the network with the wall added, so it simply takes the network's place,
		// node and wall ids included.
		Network = std::move(*Preview);

		// Joining an existing node ends the chain, otherwise the wall's end starts the next one.
		if (bPreviewEndSnapped)
		{
			Cancel();
		}
		else
		{
			Start = Endpoint{.Position = Network.FindNode(PreviewEndNode)->Position, .Node = PreviewEndNode};
			Preview.reset();
			PreviewWall = {};
			PreviewViolations.clear();
		}
		return EClickResult::Committed;
	}

	void WallPlacement::Cancel()
	{
		State = EState::PickingStart;
		Preview.reset();
		PreviewWall = {};
		PreviewViolations.clear();
	}

	double WallPlacement::GetSnapRadius(const WallNetwork& Network, NodeId Node, double SnapRadius) const
	{
		return std::max(SnapRadius, SnapWidthFactor * Network.GetWidestWall(Node));
	}

	WallPlacement::Endpoint WallPlacement::Snap(const WallNetwork& Network, Vec2 Cursor, double SnapRadius, NodeId Excluded)
	{
		SnapNode = {};
		double Nearest = std::numeric_limits<double>::max();
		for (const NodeId Id : Network.GetNodeIds())
		{
			const double ToNode = Distance(Cursor, Network.FindNode(Id)->Position);
			if (Id != Excluded && ToNode < Nearest && ToNode <= GetSnapRadius(Network, Id, SnapRadius))
			{
				SnapNode = Id;
				Nearest = ToNode;
			}
		}

		if (!SnapNode.IsValid())
		{
			return Endpoint{.Position = Cursor};
		}
		return Endpoint{.Position = Network.FindNode(SnapNode)->Position, .Node = SnapNode};
	}

	void WallPlacement::UpdatePreview(const WallNetwork& Network, Vec2 Cursor, double SnapRadius)
	{
		// Snapping the end to the start would give a wall of no length.
		const bool bStartExists = Start.Node.IsValid() && Network.FindNode(Start.Node);
		const Endpoint End = Snap(Network, Cursor, SnapRadius, bStartExists ? Start.Node : NodeId{});

		Preview = Network;
		const NodeId StartNode = bStartExists ? Start.Node : Preview->AddNode(Start.Position);
		bPreviewEndSnapped = End.Node.IsValid();
		PreviewEndNode = bPreviewEndSnapped ? End.Node : Preview->AddNode(End.Position);
		PreviewWall = Preview->AddWall(StartNode, PreviewEndNode, WidthLeft, WidthRight);

		PreviewViolations = CheckWall(*Preview, PreviewWall, Limits);
		for (const NodeId Node : {StartNode, PreviewEndNode})
		{
			for (const ELimitViolation Violation : CheckNode(*Preview, Node, Limits))
			{
				PreviewViolations.push_back(Violation);
			}
		}
	}
}
