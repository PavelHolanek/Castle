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

		/** Adds Point to the end of Path unless it coincides with the last point. */
		void AppendPoint(std::vector<Vec2>& Path, Vec2 Point)
		{
			if (Path.empty() || !IsNearlyEqual(Path.back(), Point))
			{
				Path.push_back(Point);
			}
		}

		/** The two walls around a gap and their sides facing it. */
		struct GapSides
		{
			const JunctionWall& First;
			const JunctionWall& Second;

			double FirstWidth;
			double SecondWidth;

			/** Where the raw sides end at the node. */
			Vec2 FirstStart;
			Vec2 SecondStart;
		};

		/**
		 * Where the wider wall's extended side meets the thinner wall's side.
		 * Crossing is where the two sides intersect when continued; empty when they are parallel or the straight
		 * continuation of the wider side can't reach the thinner one.
		 */
		std::pair<Vec2, EGapShape> ExtendSide(Vec2 Start, double WideWidth, Vec2 ThinStart, const JunctionWall& Thin,
			std::optional<Vec2> Crossing, const JunctionSettings& Settings)
		{
			const double MaxLength = Settings.MaxExtensionFactor * WideWidth;

			Vec2 End;
			EGapShape Shape = EGapShape::StraightExtension;
			if (Crossing && Distance(Start, *Crossing) <= MaxLength)
			{
				End = *Crossing;
			}
			else
			{
				// Turn towards the thinner wall's side so that the extension reaches it at the maximum length.
				// The thinner side starts at most both widths away from Start, so with a factor of 2 it is always in
				// reach. A smaller factor may fall short; then join the ends of the two sides directly.
				End = IntersectRayWithCircle(ThinStart, Thin.Direction, Start, MaxLength).value_or(ThinStart);
				Shape = EGapShape::BeveledExtension;
			}

			// Stop at the middle of the thinner wall's side, so that a short wall keeps room for the joint at its other end.
			const double HalfLength = 0.5 * Thin.Length;
			if (Dot(End - ThinStart, Thin.Direction) > HalfLength + DistanceTolerance)
			{
				End = ThinStart + Thin.Direction * HalfLength;
				Shape = EGapShape::ExtensionToSideMiddle;
			}
			return {End, Shape};
		}

		/** Continues the wider wall's side over the step to the thinner wall's side. */
		void SetExtension(JunctionGap& Gap, const GapSides& Sides, bool bExtendFirst, std::optional<Vec2> Crossing,
			const JunctionSettings& Settings)
		{
			if (bExtendFirst)
			{
				const auto [End, Shape] = ExtendSide(Sides.FirstStart, Sides.FirstWidth, Sides.SecondStart, Sides.Second, Crossing, Settings);
				Gap.Shape = Shape;
				Gap.Path = {Sides.FirstStart, End};
			}
			else
			{
				const auto [End, Shape] = ExtendSide(Sides.SecondStart, Sides.SecondWidth, Sides.FirstStart, Sides.First, Crossing, Settings);
				Gap.Shape = Shape;
				Gap.Path = {End, Sides.SecondStart};
			}
		}

		/** Up to 180 degrees: the sides cross, or the wider wall steps out past the thinner one. */
		void ResolveUpTo180(JunctionGap& Gap, const GapSides& Sides, std::optional<Vec2> Crossing, const JunctionSettings& Settings)
		{
			if (Crossing)
			{
				// Up to a right angle the crossing always lies on both sides. Beyond it, it lies behind the end of the
				// wider wall when the other one is thinner than -WiderWidth * cos(Angle).
				const bool bBehindFirst = Dot(*Crossing - Sides.FirstStart, Sides.First.Direction) < -DistanceTolerance;
				const bool bBehindSecond = Dot(*Crossing - Sides.SecondStart, Sides.Second.Direction) < -DistanceTolerance;
				if (!bBehindFirst && !bBehindSecond)
				{
					Gap.Shape = EGapShape::Corner;
					Gap.Path = {*Crossing};
					return;
				}
				SetExtension(Gap, Sides, bBehindFirst, Crossing, Settings);
				return;
			}

			// Walls in line: the sides are parallel and only meet when both walls are equally wide on this side.
			const double Step = Sides.FirstWidth - Sides.SecondWidth;
			if (std::abs(Step) <= DistanceTolerance)
			{
				Gap.Shape = EGapShape::Corner;
				Gap.Path = {Sides.FirstStart};
				return;
			}
			SetExtension(Gap, Sides, Step > 0.0, std::nullopt, Settings);
		}

		/** Over 180 degrees: the outer side of a bend. */
		void ResolveOver180(JunctionGap& Gap, const GapSides& Sides, std::optional<Vec2> Crossing, ECornerStyle CornerStyle,
			const JunctionSettings& Settings)
		{
			// A wall's corner lies Width * cos(Angle - 180) from the other wall's reference line. While that is more than
			// the other wall's width, the corner sticks out past its side and the step from 180 degrees goes on, always
			// beveled: the straight continuation of the wider side points away from the thinner one. It ends at
			// cos(Angle - 180) = ThinnerWidth / WiderWidth, where the bevel lines up with the thinner wall's side.
			const double Reach = std::cos(Gap.Angle - Pi);
			if (Sides.FirstWidth * Reach > Sides.SecondWidth + DistanceTolerance)
			{
				SetExtension(Gap, Sides, true, std::nullopt, Settings);
				return;
			}
			if (Sides.SecondWidth * Reach > Sides.FirstWidth + DistanceTolerance)
			{
				SetExtension(Gap, Sides, false, std::nullopt, Settings);
				return;
			}

			// From here on both corners lie within the other wall's side, so the sides, continued past the node, meet.
			Gap.Path = {Sides.FirstStart};
			if (CornerStyle == ECornerStyle::Sharp)
			{
				Gap.Shape = EGapShape::SharpOuterCorner;

				// The checks above allow the tolerance in width, so the crossing may still lie on one of the sides. Just
				// over 180 degrees that is far along it (width difference / sin(Angle - 180)), while the sides practically
				// continue each other. Leave it out then, instead of running out to it and back.
				if (Crossing && Dot(*Crossing - Sides.FirstStart, Sides.First.Direction) <= DistanceTolerance
					&& Dot(*Crossing - Sides.SecondStart, Sides.Second.Direction) <= DistanceTolerance)
				{
					AppendPoint(Gap.Path, *Crossing);
				}
			}
			else
			{
				Gap.Shape = EGapShape::BeveledOuterCorner;
			}
			AppendPoint(Gap.Path, Sides.SecondStart);
		}

		JunctionGap ResolveGap(Vec2 Position, const std::vector<JunctionWall>& Walls, size_t FirstIndex, size_t SecondIndex, double Angle,
			ECornerStyle CornerStyle, const JunctionSettings& Settings)
		{
			JunctionGap Gap{.Angle = Angle};

			const JunctionWall& First = Walls[FirstIndex];
			const JunctionWall& Second = Walls[SecondIndex];
			if (First.bIsArc || Second.bIsArc || Angle <= AngleTolerance || Angle >= 2.0 * Pi - AngleTolerance)
			{
				return Gap;
			}

			const GapSides Sides{
				.First = First,
				.Second = Second,
				.FirstWidth = First.WidthCounterclockwise,
				.SecondWidth = Second.WidthClockwise,
				.FirstStart = Position + PerpLeft(First.Direction) * First.WidthCounterclockwise,
				.SecondStart = Position - PerpLeft(Second.Direction) * Second.WidthClockwise,
			};
			const std::optional<Vec2> Crossing = IntersectLines(Sides.FirstStart, First.Direction, Sides.SecondStart, Second.Direction);

			if (Angle <= Pi + AngleTolerance)
			{
				ResolveUpTo180(Gap, Sides, Crossing, Settings);
			}
			else
			{
				ResolveOver180(Gap, Sides, Crossing, CornerStyle, Settings);
			}
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

			Result.Gaps.push_back(ResolveGap(Result.Position, Result.Walls, Index, NextIndex, Angle, Found->CornerStyle, Settings));
		}

		return Result;
	}
}
