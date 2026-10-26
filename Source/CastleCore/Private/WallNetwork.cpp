#include "CastleCore/WallNetwork.h"

#include <algorithm>
#include <cmath>

namespace Castle::Core
{
	NodeId WallNetwork::AddNode(Vec2 Position)
	{
		const NodeId Id{static_cast<uint32_t>(Nodes.size())};
		Nodes.push_back(Node{.Position = Position});
		++NodeCount;
		return Id;
	}

	WallId WallNetwork::AddWall(NodeId NodeA, NodeId NodeB, double WidthLeft, double WidthRight, double Bulge)
	{
		if (NodeA == NodeB || WidthLeft < 0.0 || WidthRight < 0.0)
		{
			return {};
		}

		Node* A = FindNodeMutable(NodeA);
		Node* B = FindNodeMutable(NodeB);
		if (!A || !B)
		{
			return {};
		}

		const WallId Id{static_cast<uint32_t>(Walls.size())};
		Walls.push_back(Wall{NodeA, NodeB, WidthLeft, WidthRight, Bulge});
		A->Walls.push_back(Id);
		B->Walls.push_back(Id);
		++WallCount;
		return Id;
	}

	NodeId WallNetwork::SplitWall(WallId Id, double T)
	{
		const std::optional<WallCurve> Reference = GetReferenceCurve(Id);
		// Written this way round to reject NaN as well.
		if (!Reference || !(T > 0.0 && T < 1.0))
		{
			return {};
		}

		const NodeId Middle = AddNode(Reference->PointAt(T));
		const WallId Rest{static_cast<uint32_t>(Walls.size())};

		// The bulge is the tangent of a quarter of the included angle; each part gets its share of the angle.
		Wall& First = *Walls[Id.Value];
		Wall Second = First;
		const double QuarterAngle = std::atan(First.Bulge);
		First.NodeB = Middle;
		First.Bulge = std::tan(T * QuarterAngle);
		Second.NodeA = Middle;
		Second.Bulge = std::tan((1.0 - T) * QuarterAngle);

		std::vector<WallId>& AtEnd = FindNodeMutable(Second.NodeB)->Walls;
		std::replace(AtEnd.begin(), AtEnd.end(), Id, Rest);
		FindNodeMutable(Middle)->Walls = {Id, Rest};

		Walls.push_back(Second);
		++WallCount;
		return Middle;
	}

	bool WallNetwork::MoveNode(NodeId Id, Vec2 Position)
	{
		Node* Found = FindNodeMutable(Id);
		if (!Found)
		{
			return false;
		}
		Found->Position = Position;
		return true;
	}

	bool WallNetwork::SetCornerStyle(NodeId Id, ECornerStyle Style)
	{
		Node* Found = FindNodeMutable(Id);
		if (!Found)
		{
			return false;
		}
		Found->CornerStyle = Style;
		return true;
	}

	bool WallNetwork::RemoveWall(WallId Id)
	{
		const Wall* Found = FindWall(Id);
		if (!Found)
		{
			return false;
		}

		for (const NodeId End : {Found->NodeA, Found->NodeB})
		{
			std::erase(FindNodeMutable(End)->Walls, Id);
		}
		Walls[Id.Value].reset();
		--WallCount;
		return true;
	}

	bool WallNetwork::RemoveNode(NodeId Id)
	{
		const Node* Found = FindNode(Id);
		if (!Found)
		{
			return false;
		}

		// Copy, because RemoveWall edits the node's list.
		const std::vector<WallId> Attached = Found->Walls;
		for (const WallId AttachedWall : Attached)
		{
			RemoveWall(AttachedWall);
		}
		Nodes[Id.Value].reset();
		--NodeCount;
		return true;
	}

	const Node* WallNetwork::FindNode(NodeId Id) const
	{
		if (Id.Value < Nodes.size() && Nodes[Id.Value])
		{
			return &*Nodes[Id.Value];
		}
		return nullptr;
	}

	const Wall* WallNetwork::FindWall(WallId Id) const
	{
		if (Id.Value < Walls.size() && Walls[Id.Value])
		{
			return &*Walls[Id.Value];
		}
		return nullptr;
	}

	NodeId WallNetwork::FindNearestNode(Vec2 Point, double MaxDistance) const
	{
		NodeId Nearest;
		double NearestDistance = 0.0;
		for (size_t Index = 0; Index < Nodes.size(); ++Index)
		{
			if (!Nodes[Index])
			{
				continue;
			}

			// On a tie the older node wins.
			const double NodeDistance = Distance(Nodes[Index]->Position, Point);
			if (NodeDistance <= MaxDistance && (!Nearest.IsValid() || NodeDistance < NearestDistance))
			{
				Nearest = NodeId{static_cast<uint32_t>(Index)};
				NearestDistance = NodeDistance;
			}
		}
		return Nearest;
	}

	std::optional<WallPoint> WallNetwork::FindWallAt(Vec2 Point, double MinReach) const
	{
		std::optional<WallPoint> Found;
		double FoundDistance = 0.0;
		for (size_t Index = 0; Index < Walls.size(); ++Index)
		{
			if (!Walls[Index])
			{
				continue;
			}

			const WallId Id{static_cast<uint32_t>(Index)};
			const WallCurve Reference = *GetReferenceCurve(Id);
			const std::optional<double> T = Reference.Project(Point);
			if (!T || *T <= 0.0 || *T >= 1.0)
			{
				continue;
			}

			const Vec2 OnReference = Reference.PointAt(*T);
			const Vec2 Away = Point - OnReference;
			const bool bLeft = Cross(Reference.TangentAt(*T), Away) >= 0.0;
			const double Reach = std::max(MinReach, bLeft ? Walls[Index]->WidthLeft : Walls[Index]->WidthRight);
			const double ToReference = Length(Away);

			// On a tie the older wall wins.
			if (ToReference <= Reach && (!Found || ToReference < FoundDistance))
			{
				Found = WallPoint{.Wall = Id, .T = *T, .Position = OnReference};
				FoundDistance = ToReference;
			}
		}
		return Found;
	}

	std::span<const WallId> WallNetwork::GetNodeWalls(NodeId Id) const
	{
		if (const Node* Found = FindNode(Id))
		{
			return Found->Walls;
		}
		return {};
	}

	double WallNetwork::GetWidestWall(NodeId Id) const
	{
		double Widest = 0.0;
		for (const WallId Attached : GetNodeWalls(Id))
		{
			const Wall& Data = *FindWall(Attached);
			Widest = std::max(Widest, Data.WidthLeft + Data.WidthRight);
		}
		return Widest;
	}

	std::optional<WallCurve> WallNetwork::GetReferenceCurve(WallId Id) const
	{
		const Wall* Found = FindWall(Id);
		if (!Found)
		{
			return std::nullopt;
		}
		return WallCurve::FromBulge(FindNode(Found->NodeA)->Position, FindNode(Found->NodeB)->Position, Found->Bulge);
	}

	std::vector<NodeId> WallNetwork::GetNodeIds() const
	{
		std::vector<NodeId> Ids;
		Ids.reserve(NodeCount);
		for (size_t Index = 0; Index < Nodes.size(); ++Index)
		{
			if (Nodes[Index])
			{
				Ids.push_back(NodeId{static_cast<uint32_t>(Index)});
			}
		}
		return Ids;
	}

	std::vector<WallId> WallNetwork::GetWallIds() const
	{
		std::vector<WallId> Ids;
		Ids.reserve(WallCount);
		for (size_t Index = 0; Index < Walls.size(); ++Index)
		{
			if (Walls[Index])
			{
				Ids.push_back(WallId{static_cast<uint32_t>(Index)});
			}
		}
		return Ids;
	}

	Node* WallNetwork::FindNodeMutable(NodeId Id)
	{
		return const_cast<Node*>(FindNode(Id));
	}
}
