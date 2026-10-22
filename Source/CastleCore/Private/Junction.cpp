#include "CastleCore/Junction.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Castle::Core
{
	namespace
	{
		/** Empty when the lines are parallel. */
		std::optional<Vec2> IntersectLines(Vec2 PointA, Vec2 DirectionA, Vec2 PointB, Vec2 DirectionB)
		{
			const double Denominator = Cross(DirectionA, DirectionB);
			if (std::abs(Denominator) <= AngleTolerance)
			{
				return std::nullopt;
			}
			return PointA + DirectionA * (Cross(PointB - PointA, DirectionB) / Denominator);
		}

		/**
		 * Point on the ray from RayStart along the unit RayDirection at Radius from Center, beyond RayStart.
		 * Empty when the ray doesn't get that far from Center.
		 */
		std::optional<Vec2> IntersectRayWithCircle(Vec2 RayStart, Vec2 RayDirection, Vec2 Center, double Radius)
		{
			const Vec2 FromCenter = RayStart - Center;
			const double Projection = Dot(FromCenter, RayDirection);
			const double Discriminant = Projection * Projection - Dot(FromCenter, FromCenter) + Radius * Radius;
			if (Discriminant < 0.0)
			{
				return std::nullopt;
			}

			const double Along = -Projection + std::sqrt(Discriminant);
			if (Along < 0.0)
			{
				return std::nullopt;
			}
			return RayStart + RayDirection * Along;
		}

		/**
		 * Where the wider wall's extended side meets the thinner wall's side.
		 * Crossing is where the two sides intersect when continued, empty when they are parallel.
		 */
		std::pair<Vec2, EExtensionShape> ExtendSide(Vec2 Start, double WideWidth, Vec2 ThinSideStart, const JunctionWall& Thin,
			std::optional<Vec2> Crossing, const JunctionSettings& Settings)
		{
			const double MaxLength = Settings.MaxExtensionFactor * WideWidth;

			Vec2 End;
			EExtensionShape Shape = EExtensionShape::Straight;
			if (Crossing && Distance(Start, *Crossing) <= MaxLength)
			{
				End = *Crossing;
			}
			else
			{
				// Turn towards the thinner wall's side so that the extension reaches it at the maximum length.
				// The thinner side starts at most both widths away from Start, so with a factor of 2 it is always in
				// reach. A smaller factor may fall short; then join the ends of the two sides directly.
				End = IntersectRayWithCircle(ThinSideStart, Thin.Direction, Start, MaxLength).value_or(ThinSideStart);
				Shape = EExtensionShape::Beveled;
			}

			// Stop at the middle of the thinner wall's side, so that a short wall keeps room for the joint at its other end.
			const double HalfLength = 0.5 * Thin.Length;
			if (Dot(End - ThinSideStart, Thin.Direction) > HalfLength + DistanceTolerance)
			{
				End = ThinSideStart + Thin.Direction * HalfLength;
				Shape = EExtensionShape::ToSideMiddle;
			}
			return {End, Shape};
		}

		JunctionGap ResolveGap(Vec2 Position, const std::vector<JunctionWall>& Walls, size_t FirstIndex, size_t SecondIndex, double Angle,
			const JunctionSettings& Settings)
		{
			JunctionGap Gap{.Angle = Angle};

			const JunctionWall& First = Walls[FirstIndex];
			const JunctionWall& Second = Walls[SecondIndex];
			if (First.bIsArc || Second.bIsArc || Angle <= AngleTolerance || Angle > Pi + AngleTolerance)
			{
				return Gap;
			}

			// Where the raw sides end at the node.
			const Vec2 FirstSideStart = Position + PerpLeft(First.Direction) * First.WidthCounterclockwise;
			const Vec2 SecondSideStart = Position - PerpLeft(Second.Direction) * Second.WidthClockwise;
			const std::optional<Vec2> Crossing = IntersectLines(FirstSideStart, First.Direction, SecondSideStart, Second.Direction);

			// Up to a right angle the sides always cross on both walls. Beyond it the crossing lies behind the end of the
			// wider wall when the other one is thinner than -WiderWidth * cos(Angle). At 180 degrees the sides are parallel
			// and only meet when both walls are equally wide on this side.
			bool bExtendFirst = false;
			if (Crossing)
			{
				bExtendFirst = Dot(*Crossing - FirstSideStart, First.Direction) < -DistanceTolerance;
				if (!bExtendFirst && Dot(*Crossing - SecondSideStart, Second.Direction) >= -DistanceTolerance)
				{
					Gap.Corner = Crossing;
					return Gap;
				}
			}
			else
			{
				const double Step = First.WidthCounterclockwise - Second.WidthClockwise;
				if (std::abs(Step) <= DistanceTolerance)
				{
					Gap.Corner = FirstSideStart;
					return Gap;
				}
				bExtendFirst = Step > 0.0;
			}

			const Vec2 Start = bExtendFirst ? FirstSideStart : SecondSideStart;
			const auto [End, Shape] = bExtendFirst
				? ExtendSide(Start, First.WidthCounterclockwise, SecondSideStart, Second, Crossing, Settings)
				: ExtendSide(Start, Second.WidthClockwise, FirstSideStart, First, Crossing, Settings);

			Gap.Corner = End;
			Gap.Extension = SideExtension{.WallIndex = bExtendFirst ? FirstIndex : SecondIndex, .Start = Start, .Shape = Shape};
			return Gap;
		}
	}

	std::optional<Junction> BuildJunction(const WallNetwork& Network, NodeId Id, const JunctionSettings& Settings)
	{
		const Node* Found = Network.FindNode(Id);
		if (!Found)
		{
			return std::nullopt;
		}

		Junction Result{.Node = Id, .Position = Found->Position};
		for (const WallId AttachedWall : Found->Walls)
		{
			const Wall& Data = *Network.FindWall(AttachedWall);
			const WallCurve Reference = *Network.GetReferenceCurve(AttachedWall);
			const bool bStartsHere = Data.NodeA == Id;
			const Vec2 Direction = bStartsHere ? Reference.TangentAt(0.0) : -Reference.TangentAt(1.0);

			double DirectionAngle = std::atan2(Direction.Y, Direction.X);
			if (DirectionAngle < 0.0)
			{
				DirectionAngle += 2.0 * Pi;
			}

			Result.Walls.push_back(JunctionWall{
				.Id = AttachedWall,
				.Direction = Direction,
				.DirectionAngle = DirectionAngle,
				.WidthCounterclockwise = bStartsHere ? Data.WidthLeft : Data.WidthRight,
				.WidthClockwise = bStartsHere ? Data.WidthRight : Data.WidthLeft,
				.Length = Reference.GetLength(),
				.bIsArc = Reference.IsArc(),
			});
		}

		std::stable_sort(Result.Walls.begin(), Result.Walls.end(), [](const JunctionWall& A, const JunctionWall& B)
		{
			return A.DirectionAngle < B.DirectionAngle;
		});

		const size_t Count = Result.Walls.size();
		for (size_t Index = 0; Index < Count; ++Index)
		{
			const size_t NextIndex = (Index + 1) % Count;

			// The last gap wraps around past +X.
			double Angle = Result.Walls[NextIndex].DirectionAngle - Result.Walls[Index].DirectionAngle;
			if (Index + 1 == Count)
			{
				Angle += 2.0 * Pi;
			}

			Result.Gaps.push_back(ResolveGap(Result.Position, Result.Walls, Index, NextIndex, Angle, Settings));
		}

		return Result;
	}
}
