#include "CastleCore/Junction.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace Castle::Core;

namespace
{
	constexpr double Tolerance = 1e-9;

	void ExpectNear(Vec2 Actual, Vec2 Expected)
	{
		EXPECT_NEAR(Actual.X, Expected.X, Tolerance);
		EXPECT_NEAR(Actual.Y, Expected.Y, Tolerance);
	}

	void ExpectPath(const JunctionGap& Gap, EGapShape Shape, const std::vector<Vec2>& Path)
	{
		EXPECT_EQ(Gap.Shape, Shape);
		ASSERT_EQ(Gap.Path.size(), Path.size());
		for (size_t Index = 0; Index < Path.size(); ++Index)
		{
			SCOPED_TRACE(Index);
			ExpectNear(Gap.Path[Index], Path[Index]);
		}
	}

	Vec2 Direction(double Degrees)
	{
		return PointOnCircle({0.0, 0.0}, 1.0, DegreesToRadians(Degrees));
	}

	/** Straight walls around a node at the origin. */
	struct Star
	{
		WallNetwork Network;
		NodeId Center = Network.AddNode({0.0, 0.0});

		/** Wall leaving the node in the given direction (degrees). */
		WallId AddOutgoing(double Degrees, double WidthLeft, double WidthRight, double Length = 1000.0)
		{
			return Network.AddWall(Center, Network.AddNode(Direction(Degrees) * Length), WidthLeft, WidthRight);
		}

		/** Wall arriving at the node from the given direction (degrees). */
		WallId AddIncoming(double Degrees, double WidthLeft, double WidthRight)
		{
			return Network.AddWall(Network.AddNode(Direction(Degrees) * 1000.0), Center, WidthLeft, WidthRight);
		}

		Junction Build(const JunctionSettings& Settings = {}) const
		{
			return *BuildJunction(Network, Center, Settings);
		}
	};

	/** Distance of Point from the line of the clockwise side of a straight wall leaving the origin. */
	double DistanceFromClockwiseSide(Vec2 Point, double Degrees, double Width)
	{
		return Cross(Direction(Degrees), Point - (-PerpLeft(Direction(Degrees)) * Width));
	}

	/** Checks that Point lies on the clockwise side of a straight wall leaving the origin, beyond the node. */
	void ExpectOnClockwiseSide(Vec2 Point, double Degrees, double Width)
	{
		const Vec2 SideStart = -PerpLeft(Direction(Degrees)) * Width;
		EXPECT_NEAR(DistanceFromClockwiseSide(Point, Degrees, Width), 0.0, 1e-6);
		EXPECT_GT(Dot(Direction(Degrees), Point - SideStart), 0.0);
	}
}

TEST(Junction, UnknownNodeHasNoJunction)
{
	const WallNetwork Network;

	EXPECT_FALSE(BuildJunction(Network, NodeId{}));
}

TEST(Junction, SingleWallLeavesFullTurnUnresolved)
{
	Star Joint;
	Joint.AddOutgoing(30.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 1u);
	EXPECT_NEAR(Result.Gaps[0].Angle, 2.0 * Pi, Tolerance);
	EXPECT_EQ(Result.Gaps[0].Shape, EGapShape::Unresolved);
	EXPECT_TRUE(Result.Gaps[0].Path.empty());
}

TEST(Junction, WallsAreSortedCounterclockwise)
{
	Star Joint;
	const WallId At200 = Joint.AddOutgoing(200.0, 100.0, 100.0);
	const WallId At20 = Joint.AddOutgoing(20.0, 100.0, 100.0);
	const WallId At100 = Joint.AddIncoming(100.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Walls.size(), 3u);
	EXPECT_EQ(Result.Walls[0].Id, At20);
	EXPECT_EQ(Result.Walls[1].Id, At100);
	EXPECT_EQ(Result.Walls[2].Id, At200);

	ASSERT_EQ(Result.Gaps.size(), 3u);
	EXPECT_NEAR(Result.Gaps[0].Angle, DegreesToRadians(80.0), Tolerance);
	EXPECT_NEAR(Result.Gaps[1].Angle, DegreesToRadians(100.0), Tolerance);
	EXPECT_NEAR(Result.Gaps[2].Angle, DegreesToRadians(180.0), Tolerance);
}

TEST(Junction, WallEndingAtNodeIsSeenBackwards)
{
	Star Joint;
	Joint.AddIncoming(90.0, 10.0, 30.0);

	const Junction Result = Joint.Build();
	const JunctionWall& Seen = Result.Walls[0];
	ExpectNear(Seen.Direction, {0.0, 1.0});
	EXPECT_NEAR(Seen.DirectionAngle, Pi / 2.0, Tolerance);
	EXPECT_EQ(Seen.WidthCounterclockwise, 30.0);
	EXPECT_EQ(Seen.WidthClockwise, 10.0);
}

// Up to 90 degrees

TEST(Junction, AcuteCornerOfEqualWalls)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(60.0, 100.0, 100.0);

	// On the bisector, at half the width divided by sin(30 degrees).
	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {Direction(30.0) * 200.0});
}

TEST(Junction, AcuteCornerUsesTheSidesFacingTheGap)
{
	// Only the first wall's left side and the second wall's right side face the 45 degree gap.
	// 10 from the first wall and 20 from the second: x = (20 + 10 cos 45) / sin 45.
	Star Joint;
	Joint.AddOutgoing(0.0, 10.0, 90.0);
	Joint.AddOutgoing(45.0, 70.0, 20.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {{20.0 * std::sqrt(2.0) + 10.0, 10.0}});
}

TEST(Junction, AcuteCornerWithWallEndingAtNode)
{
	// Same as above, but the second wall runs towards the node, so its left side faces the gap.
	Star Joint;
	Joint.AddOutgoing(0.0, 10.0, 90.0);
	Joint.AddIncoming(45.0, 20.0, 70.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {{20.0 * std::sqrt(2.0) + 10.0, 10.0}});
}

TEST(Junction, RightAngleCorner)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 30.0, 30.0);
	Joint.AddOutgoing(90.0, 50.0, 50.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {{50.0, 30.0}});
}

TEST(Junction, TJunctionHasTwoRightCorners)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddIncoming(180.0, 150.0, 150.0);
	Joint.AddOutgoing(90.0, 60.0, 60.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 3u);
	ExpectPath(Result.Gaps[0], EGapShape::Corner, {{60.0, 150.0}});
	ExpectPath(Result.Gaps[1], EGapShape::Corner, {{-60.0, 150.0}});

	// The through wall's sides simply continue.
	EXPECT_NEAR(Result.Gaps[2].Angle, Pi, Tolerance);
	ExpectPath(Result.Gaps[2], EGapShape::Corner, {{0.0, -150.0}});
}

TEST(Junction, AcuteAndRightCornersNeverNeedExtension)
{
	for (const double Degrees : {30.0, 60.0, 90.0})
	{
		Star Joint;
		Joint.AddOutgoing(0.0, 300.0, 300.0);
		Joint.AddOutgoing(Degrees, 5.0, 5.0);
		SCOPED_TRACE(Degrees);

		EXPECT_EQ(Joint.Build().Gaps[0].Shape, EGapShape::Corner);
	}
}

// 90 to 180 degrees

TEST(Junction, ObtuseCornerOfEqualWalls)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 100.0, 100.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {Direction(60.0) * (100.0 / std::sin(DegreesToRadians(60.0)))});
}

TEST(Junction, ObtuseCornerOnWiderSide)
{
	// At 120 degrees the step appears below 100 * -cos(120) = 50; 80 is still enough.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 80.0, 80.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {{(80.0 - 50.0) / std::sin(DegreesToRadians(120.0)), 100.0}});
}

TEST(Junction, ObtuseCornerExactlyAtStepNeedsNoExtension)
{
	// 100 * -cos(120) = 50: the thinner side meets the wider one right at its end.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 50.0, 50.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::Corner, {{0.0, 100.0}});
}

TEST(Junction, ObtuseCornerBehindWiderWallExtendsItsSide)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 20.0, 20.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::StraightExtension,
		{{0.0, 100.0}, {(20.0 - 50.0) / std::sin(DegreesToRadians(120.0)), 100.0}});
}

TEST(Junction, ObtuseCornerExtendsTheSecondWallWhenItIsWider)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 20.0, 20.0);
	Joint.AddIncoming(120.0, 100.0, 100.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::StraightExtension,
		{{(100.0 - 10.0) / std::sin(DegreesToRadians(120.0)), 20.0}, Direction(30.0) * 100.0});
}

TEST(Junction, LongExtensionIsBeveledToTwiceTheWiderWidth)
{
	// At 170 degrees the straight extension would be (150 * -cos(170) - 80) / sin(170), about 390.
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(170.0, 80.0, 80.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	EXPECT_EQ(Gap.Shape, EGapShape::BeveledExtension);
	ASSERT_EQ(Gap.Path.size(), 2u);
	ExpectNear(Gap.Path[0], {0.0, 150.0});
	EXPECT_NEAR(Distance(Gap.Path[0], Gap.Path[1]), 300.0, 1e-6);
	ExpectOnClockwiseSide(Gap.Path[1], 170.0, 80.0);
}

TEST(Junction, MaxExtensionFactorIsAdjustable)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(170.0, 80.0, 80.0);

	const JunctionGap Gap = Joint.Build(JunctionSettings{.MaxExtensionFactor = 1.0}).Gaps[0];
	EXPECT_EQ(Gap.Shape, EGapShape::BeveledExtension);
	ASSERT_EQ(Gap.Path.size(), 2u);
	EXPECT_NEAR(Distance(Gap.Path[0], Gap.Path[1]), 150.0, 1e-6);
	ExpectOnClockwiseSide(Gap.Path[1], 170.0, 80.0);
}

TEST(Junction, ExtensionStopsAtMiddleOfShortThinWall)
{
	// The straight extension would meet the thin side about 104 along it, past the middle of a 150 long wall.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 20.0, 20.0, 150.0);

	ExpectPath(Joint.Build().Gaps[0], EGapShape::ExtensionToSideMiddle,
		{{0.0, 100.0}, -PerpLeft(Direction(120.0)) * 20.0 + Direction(120.0) * 75.0});
}

// 180 degrees

TEST(Junction, WallsInLineWithEqualWidthsSimplyContinue)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(180.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ExpectPath(Result.Gaps[0], EGapShape::Corner, {{0.0, 100.0}});
	ExpectPath(Result.Gaps[1], EGapShape::Corner, {{0.0, -100.0}});
}

TEST(Junction, WallsInLineWithDifferentWidthsAreBeveledOnBothSides)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddIncoming(180.0, 80.0, 80.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 2u);
	const double Reach = std::sqrt(300.0 * 300.0 - 70.0 * 70.0);
	EXPECT_NEAR(Result.Gaps[0].Angle, Pi, Tolerance);
	ExpectPath(Result.Gaps[0], EGapShape::BeveledExtension, {{0.0, 150.0}, {-Reach, 80.0}});
	ExpectPath(Result.Gaps[1], EGapShape::BeveledExtension, {{-Reach, -80.0}, {0.0, -150.0}});
}

// Over 180 degrees

TEST(Junction, StepOver180IsBeveledAsAt180)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(200.0, 80.0, 80.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	EXPECT_NEAR(Gap.Angle, DegreesToRadians(200.0), Tolerance);
	EXPECT_EQ(Gap.Shape, EGapShape::BeveledExtension);
	ASSERT_EQ(Gap.Path.size(), 2u);
	ExpectNear(Gap.Path[0], {0.0, 150.0});
	EXPECT_NEAR(Distance(Gap.Path[0], Gap.Path[1]), 300.0, 1e-6);
	ExpectOnClockwiseSide(Gap.Path[1], 200.0, 80.0);
}

TEST(Junction, StepOver180EndsWhereBevelLinesUpWithThinnerSide)
{
	// cos(Angle - 180) = 80 / 150 at about 237.8 degrees.
	for (const auto& [Degrees, Shape] : {std::pair{235.0, EGapShape::BeveledExtension}, std::pair{240.0, EGapShape::SharpOuterCorner}})
	{
		Star Joint;
		Joint.AddOutgoing(0.0, 150.0, 150.0);
		Joint.AddOutgoing(Degrees, 80.0, 80.0);
		SCOPED_TRACE(Degrees);

		EXPECT_EQ(Joint.Build().Gaps[0].Shape, Shape);
	}
}

TEST(Junction, AtCriticalAngleTheOutlineFollowsTheThinnerSide)
{
	// cos(225 - 180) = 141.42 / 200: the wider wall's corner lies on the thinner wall's side line.
	// Whichever rule applies there, the whole connection runs along that line.
	const double ThinWidth = 200.0 * std::cos(DegreesToRadians(45.0));
	for (const ECornerStyle Style : {ECornerStyle::Sharp, ECornerStyle::Beveled})
	{
		Star Joint;
		Joint.AddOutgoing(0.0, 200.0, 200.0);
		Joint.AddOutgoing(225.0, ThinWidth, ThinWidth);
		Joint.Network.SetCornerStyle(Joint.Center, Style);

		const JunctionGap Gap = Joint.Build().Gaps[0];
		ASSERT_FALSE(Gap.Path.empty());
		ExpectNear(Gap.Path.front(), {0.0, 200.0});
		for (const Vec2& Point : Gap.Path)
		{
			EXPECT_NEAR(DistanceFromClockwiseSide(Point, 225.0, ThinWidth), 0.0, 1e-6);
		}
	}
}

TEST(Junction, SharpOuterCornerOfL)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(270.0, 60.0, 60.0);

	const Junction Result = Joint.Build();
	ExpectPath(Result.Gaps[0], EGapShape::SharpOuterCorner, {{0.0, 150.0}, {-60.0, 150.0}, {-60.0, 0.0}});
	ExpectPath(Result.Gaps[1], EGapShape::Corner, {{60.0, -150.0}});
}

TEST(Junction, BeveledOuterCornerOfL)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(270.0, 60.0, 60.0);
	Joint.Network.SetCornerStyle(Joint.Center, ECornerStyle::Beveled);

	const Junction Result = Joint.Build();
	ExpectPath(Result.Gaps[0], EGapShape::BeveledOuterCorner, {{0.0, 150.0}, {-60.0, 0.0}});

	// The style only applies over 180 degrees.
	ExpectPath(Result.Gaps[1], EGapShape::Corner, {{60.0, -150.0}});
}

TEST(Junction, SharpOuterCornerOfEqualWallsLiesOnBisector)
{
	// Outside a 60 degree bend: half the width divided by sin(30 degrees), opposite the inner corner.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(60.0, 100.0, 100.0);

	const JunctionGap Gap = Joint.Build().Gaps[1];
	EXPECT_NEAR(Gap.Angle, DegreesToRadians(300.0), Tolerance);
	ExpectPath(Gap, EGapShape::SharpOuterCorner, {Direction(150.0) * 100.0, Direction(210.0) * 200.0, Direction(270.0) * 100.0});
}

// Not resolved yet

TEST(Junction, ArcsAreNotResolvedYet)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.Network.AddWall(Joint.Center, Joint.Network.AddNode({500.0, 1000.0}), 100.0, 100.0, 0.2);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 2u);
	EXPECT_EQ(Result.Gaps[0].Shape, EGapShape::Unresolved);
	EXPECT_EQ(Result.Gaps[1].Shape, EGapShape::Unresolved);
}
