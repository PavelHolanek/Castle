#include "CastleCore/FloorPlan.h"

#include <algorithm>
#include <cmath>

namespace Castle::Core
{
	namespace
	{
		/**
		 * Removes repeated points and points lying on the line through their neighbours. That includes spikes running
		 * out and back along one line, which enclose no area.
		 */
		void Simplify(std::vector<Vec2>& Polygon)
		{
			bool bChanged = true;
			while (bChanged && Polygon.size() >= 3)
			{
				bChanged = false;
				const size_t Count = Polygon.size();
				for (size_t Index = 0; Index < Count; ++Index)
				{
					const Vec2 Previous = Polygon[(Index + Count - 1) % Count];
					const Vec2 Current = Polygon[Index];
					const Vec2 Next = Polygon[(Index + 1) % Count];

					const double Span = Distance(Previous, Next);
					const bool bRedundant = IsNearlyEqual(Current, Next)
						|| Span <= DistanceTolerance
						|| std::abs(Cross(Next - Previous, Current - Previous)) / Span <= DistanceTolerance;
					if (bRedundant)
					{
						Polygon.erase(Polygon.begin() + static_cast<std::ptrdiff_t>(Index));
						bChanged = true;
						break;
					}
				}
			}
		}

		bool Contains(const std::vector<NodeId>& Nodes, NodeId Id)
		{
			return std::find(Nodes.begin(), Nodes.end(), Id) != Nodes.end();
		}

		bool IsResolved(const Junction& NodeJunction)
		{
			return std::none_of(NodeJunction.Gaps.begin(), NodeJunction.Gaps.end(), [](const JunctionGap& Gap)
			{
				return Gap.Shape == EGapShape::Unresolved;
			});
		}
	}

	FloorPlan BuildFloorPlan(const WallNetwork& Network, const FloorPlanSettings& Settings)
	{
		FloorPlan Plan;

		// Bodies by wall id, to fill in the cuts node by node.
		std::vector<size_t> BodyIndexById;
		for (const WallId Id : Network.GetWallIds())
		{
			if (BodyIndexById.size() <= Id.Value)
			{
				BodyIndexById.resize(static_cast<size_t>(Id.Value) + 1);
			}
			BodyIndexById[Id.Value] = Plan.Walls.size();
			Plan.Walls.push_back(WallBody{.Wall = Id});
		}

		for (const NodeId Id : Network.GetNodeIds())
		{
			// A free end keeps its cut at the node: a plain cap.
			if (Network.GetNodeWalls(Id).size() < 2)
			{
				continue;
			}

			const Junction NodeJunction = *BuildJunction(Network, Id, Settings.Junctions);
			if (Contains(Settings.SkippedNodes, Id) || !IsResolved(NodeJunction))
			{
				Plan.UnresolvedNodes.push_back(Id);
				continue;
			}

			// Wall I's counterclockwise side meets the joint at the start of gap I's path, its clockwise side at the end
			// of the previous gap's path. Going around the node counterclockwise, the piece's outline crosses each wall
			// along its cut, from the clockwise side to the counterclockwise one, and follows the gap's path to the next wall.
			std::vector<Vec2> Polygon;
			const size_t Count = NodeJunction.Walls.size();
			for (size_t Index = 0; Index < Count; ++Index)
			{
				const JunctionWall& Attached = NodeJunction.Walls[Index];
				const JunctionGap& CounterclockwiseGap = NodeJunction.Gaps[Index];
				const JunctionGap& ClockwiseGap = NodeJunction.Gaps[(Index + Count - 1) % Count];

				const double Cut = std::max({0.0,
					Dot(CounterclockwiseGap.Path.front() - NodeJunction.Position, Attached.Direction),
					Dot(ClockwiseGap.Path.back() - NodeJunction.Position, Attached.Direction)});

				WallBody& Body = Plan.Walls[BodyIndexById[Attached.Id.Value]];
				(Network.FindWall(Attached.Id)->NodeA == Id ? Body.StartCut : Body.EndCut) = Cut;

				const Vec2 CutCenter = NodeJunction.Position + Attached.Direction * Cut;
				const Vec2 Across = PerpLeft(Attached.Direction);
				Polygon.push_back(CutCenter - Across * Attached.WidthClockwise);
				Polygon.push_back(CutCenter + Across * Attached.WidthCounterclockwise);
				Polygon.insert(Polygon.end(), CounterclockwiseGap.Path.begin(), CounterclockwiseGap.Path.end());
			}

			Simplify(Polygon);
			if (Polygon.size() >= 3 && GetSignedArea(Polygon) > DistanceTolerance)
			{
				Plan.Nodes.push_back(NodePiece{.Node = Id, .Polygon = std::move(Polygon)});
			}
		}

		for (WallBody& Body : Plan.Walls)
		{
			const Wall& Data = *Network.FindWall(Body.Wall);
			const WallCurve Reference = *Network.GetReferenceCurve(Body.Wall);
			const double Length = Reference.GetLength();
			if (Body.StartCut + Body.EndCut >= Length - DistanceTolerance)
			{
				Body.bConsumedByJoints = true;
				continue;
			}

			// Offsets share the reference line's parametrization, so a cut at one T runs across the wall.
			const double T0 = Body.StartCut / Length;
			const double T1 = 1.0 - Body.EndCut / Length;
			if (const std::optional<WallCurve> Left = Reference.Offset(Data.WidthLeft))
			{
				Body.Left = Left->Segment(T0, T1);
			}
			if (const std::optional<WallCurve> Right = Reference.Offset(-Data.WidthRight))
			{
				Body.Right = Right->Segment(T0, T1);
			}
		}

		return Plan;
	}

	std::vector<Vec2> GetBodyPolygon(const WallBody& Body, double MaxAngleStep)
	{
		if (!Body.Left || !Body.Right)
		{
			return {};
		}

		// Out along the right side, back along the left one.
		std::vector<Vec2> Polygon = Body.Right->Tessellate(MaxAngleStep);
		const std::vector<Vec2> Left = Body.Left->Tessellate(MaxAngleStep);
		Polygon.insert(Polygon.end(), Left.rbegin(), Left.rend());
		return Polygon;
	}
}
