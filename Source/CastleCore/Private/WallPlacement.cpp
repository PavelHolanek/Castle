#include "CastleCore/WallPlacement.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace Castle::Core
{
	namespace
	{
		/** Adds Found to Violations, each limit once even when it is broken in several places. */
		void AddViolations(std::vector<ELimitViolation>& Violations, const std::vector<ELimitViolation>& Found)
		{
			for (const ELimitViolation Violation : Found)
			{
				if (std::find(Violations.begin(), Violations.end(), Violation) == Violations.end())
				{
					Violations.push_back(Violation);
				}
			}
		}
	}

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

		// Joining an existing node or wall ends the chain, otherwise the wall's end starts the next one.
		if (bPreviewEndJoins)
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
		SnapWall.reset();
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

		if (SnapNode.IsValid())
		{
			return Endpoint{.Position = Network.FindNode(SnapNode)->Position, .Node = SnapNode};
		}

		SnapWall = Network.FindWallAt(Cursor, SnapRadius);
		if (SnapWall)
		{
			return Endpoint{.Position = SnapWall->Position, .Wall = SnapWall->Wall, .WallT = SnapWall->T};
		}
		return Endpoint{.Position = Cursor};
	}

	NodeId WallPlacement::MakeNode(WallNetwork& Network, const Endpoint& Point)
	{
		if (Point.Node.IsValid() && Network.FindNode(Point.Node))
		{
			return Point.Node;
		}
		if (Point.Wall.IsValid())
		{
			// Fails for a wall removed meanwhile, or a split right at the end of a part, which then gets a node of its own.
			if (const NodeId Split = Network.SplitWall(Point.Wall, Point.WallT); Split.IsValid())
			{
				return Split;
			}
		}
		return Network.AddNode(Point.Position);
	}

	void WallPlacement::UpdatePreview(const WallNetwork& Network, Vec2 Cursor, double SnapRadius)
	{
		// Snapping the end to the start would give a wall of no length.
		const bool bStartExists = Start.Node.IsValid() && Network.FindNode(Start.Node);
		const Endpoint End = Snap(Network, Cursor, SnapRadius, bStartExists ? Start.Node : NodeId{});

		Preview = Network;
		const NodeId StartNode = MakeNode(*Preview, Start);

		// Both ends on the same wall: the start split it into the part before, which kept the wall's id, and the part
		// after it. The end splits the part it lies on, measured along that part.
		Endpoint EndOnPart = End;
		const std::span<const WallId> StartWalls = Preview->GetNodeWalls(StartNode);
		if (End.Wall.IsValid() && End.Wall == Start.Wall && StartWalls.size() == 2)
		{
			if (End.WallT < Start.WallT)
			{
				EndOnPart.WallT = End.WallT / Start.WallT;
			}
			else
			{
				EndOnPart.Wall = StartWalls[1];
				EndOnPart.WallT = (End.WallT - Start.WallT) / (1.0 - Start.WallT);
			}
		}

		PreviewEndNode = MakeNode(*Preview, EndOnPart);
		bPreviewEndJoins = End.Node.IsValid() || End.Wall.IsValid();
		PreviewWall = Preview->AddWall(StartNode, PreviewEndNode, WidthLeft, WidthRight);

		PreviewViolations.clear();
		AddViolations(PreviewViolations, CheckWall(*Preview, PreviewWall, Limits));
		for (const NodeId Node : {StartNode, PreviewEndNode})
		{
			AddViolations(PreviewViolations, CheckNode(*Preview, Node, Limits));

			// A new node may split a wall, whose parts have to keep within the limits too (e.g. not get too short).
			if (!Network.FindNode(Node))
			{
				for (const WallId Attached : Preview->GetNodeWalls(Node))
				{
					AddViolations(PreviewViolations, CheckWall(*Preview, Attached, Limits));
				}
			}
		}
	}
}
