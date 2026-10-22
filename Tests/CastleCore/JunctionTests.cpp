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

	/** Checks that Point lies on the clockwise side of a straight wall leaving the origin, beyond the node. */
	void ExpectOnClockwiseSide(Vec2 Point, double Degrees, double Width)
	{
		const Vec2 SideStart = -PerpLeft(Direction(Degrees)) * Width;
		EXPECT_NEAR(Cross(Direction(Degrees), Point - SideStart), 0.0, 1e-6);
		EXPECT_GT(Dot(Direction(Degrees), Point - SideStart), 0.0);
	}
}

TEST(Junction, UnknownNodeHasNoJunction)
{
	const WallNetwork Network;

	EXPECT_FALSE(BuildJunction(Network, NodeId{}));
}

TEST(Junction, SingleWallLeavesFullTurn)
{
	Star Joint;
	Joint.AddOutgoing(30.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 1u);
	EXPECT_NEAR(Result.Gaps[0].Angle, 2.0 * Pi, Tolerance);
	EXPECT_FALSE(Result.Gaps[0].Corner);
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

TEST(Junction, AcuteCornerOfEqualWalls)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(60.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ASSERT_TRUE(Result.Gaps[0].Corner);

	// On the bisector, at half the width divided by sin(30 degrees).
	ExpectNear(*Result.Gaps[0].Corner, Direction(30.0) * 200.0);

	// The other side of the joint (300 degrees) is not handled yet.
	EXPECT_FALSE(Result.Gaps[1].Corner);
}

TEST(Junction, AcuteCornerUsesTheSidesFacingTheGap)
{
	// Only the first wall's left side and the second wall's right side face the 45 degree gap.
	Star Joint;
	Joint.AddOutgoing(0.0, 10.0, 90.0);
	Joint.AddOutgoing(45.0, 70.0, 20.0);

	const Junction Result = Joint.Build();
	ASSERT_TRUE(Result.Gaps[0].Corner);

	// 10 from the first wall and 20 from the second: x = (20 + 10 cos 45) / sin 45.
	ExpectNear(*Result.Gaps[0].Corner, {20.0 * std::sqrt(2.0) + 10.0, 10.0});
}

TEST(Junction, AcuteCornerWithWallEndingAtNode)
{
	// Same as above, but the second wall runs towards the node, so its left side faces the gap.
	Star Joint;
	Joint.AddOutgoing(0.0, 10.0, 90.0);
	Joint.AddIncoming(45.0, 20.0, 70.0);

	const Junction Result = Joint.Build();
	ASSERT_TRUE(Result.Gaps[0].Corner);
	ExpectNear(*Result.Gaps[0].Corner, {20.0 * std::sqrt(2.0) + 10.0, 10.0});
}

TEST(Junction, RightAngleCorner)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 30.0, 30.0);
	Joint.AddOutgoing(90.0, 50.0, 50.0);

	ExpectNear(*Joint.Build().Gaps[0].Corner, {50.0, 30.0});
}

TEST(Junction, TJunctionHasTwoRightCorners)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddIncoming(180.0, 150.0, 150.0);
	Joint.AddOutgoing(90.0, 60.0, 60.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 3u);
	ExpectNear(*Result.Gaps[0].Corner, {60.0, 150.0});
	ExpectNear(*Result.Gaps[1].Corner, {-60.0, 150.0});
	EXPECT_NEAR(Result.Gaps[2].Angle, Pi, Tolerance);

	// The through wall's sides simply continue.
	ExpectNear(*Result.Gaps[2].Corner, {0.0, -150.0});
	EXPECT_FALSE(Result.Gaps[2].Extension);
}

TEST(Junction, ObtuseCornerOfEqualWalls)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ASSERT_TRUE(Result.Gaps[0].Corner);
	ExpectNear(*Result.Gaps[0].Corner, Direction(60.0) * (100.0 / std::sin(DegreesToRadians(60.0))));
	EXPECT_FALSE(Result.Gaps[0].Extension);

	// The 240 degree side is not handled yet.
	EXPECT_FALSE(Result.Gaps[1].Corner);
}

TEST(Junction, ObtuseCornerOnWiderSide)
{
	// At 120 degrees the step appears below 100 * -cos(120) = 50; 80 is still enough.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 80.0, 80.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	ASSERT_TRUE(Gap.Corner);
	ExpectNear(*Gap.Corner, {(80.0 - 50.0) / std::sin(DegreesToRadians(120.0)), 100.0});
	EXPECT_FALSE(Gap.Extension);
}

TEST(Junction, ObtuseCornerBehindWiderWallExtendsItsSide)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 20.0, 20.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	ASSERT_TRUE(Gap.Corner);
	ExpectNear(*Gap.Corner, {(20.0 - 50.0) / std::sin(DegreesToRadians(120.0)), 100.0});

	ASSERT_TRUE(Gap.Extension);
	EXPECT_EQ(Gap.Extension->WallIndex, 0u);
	EXPECT_EQ(Gap.Extension->Shape, EExtensionShape::Straight);
	ExpectNear(Gap.Extension->Start, {0.0, 100.0});
}

TEST(Junction, LongExtensionIsBeveledToTwiceTheWiderWidth)
{
	// At 170 degrees the straight extension would be (150 * -cos(170) - 80) / sin(170), about 390.
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(170.0, 80.0, 80.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	ASSERT_TRUE(Gap.Corner);
	ASSERT_TRUE(Gap.Extension);
	EXPECT_EQ(Gap.Extension->WallIndex, 0u);
	EXPECT_EQ(Gap.Extension->Shape, EExtensionShape::Beveled);
	ExpectNear(Gap.Extension->Start, {0.0, 150.0});
	EXPECT_NEAR(Distance(Gap.Extension->Start, *Gap.Corner), 300.0, 1e-6);
	ExpectOnClockwiseSide(*Gap.Corner, 170.0, 80.0);
}

TEST(Junction, MaxExtensionFactorIsAdjustable)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddOutgoing(170.0, 80.0, 80.0);

	const JunctionGap Gap = Joint.Build(JunctionSettings{.MaxExtensionFactor = 1.0}).Gaps[0];
	ASSERT_TRUE(Gap.Extension);
	EXPECT_EQ(Gap.Extension->Shape, EExtensionShape::Beveled);
	EXPECT_NEAR(Distance(Gap.Extension->Start, *Gap.Corner), 150.0, 1e-6);
	ExpectOnClockwiseSide(*Gap.Corner, 170.0, 80.0);
}

TEST(Junction, WallsInLineWithDifferentWidthsAreBeveledOnBothSides)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 150.0, 150.0);
	Joint.AddIncoming(180.0, 80.0, 80.0);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 2u);
	const double Reach = std::sqrt(300.0 * 300.0 - 70.0 * 70.0);
	for (size_t Index = 0; Index < 2; ++Index)
	{
		const JunctionGap& Gap = Result.Gaps[Index];
		const double Side = Index == 0 ? 1.0 : -1.0;
		SCOPED_TRACE(Index);

		EXPECT_NEAR(Gap.Angle, Pi, Tolerance);
		ASSERT_TRUE(Gap.Corner);
		ASSERT_TRUE(Gap.Extension);
		EXPECT_EQ(Result.Walls[Gap.Extension->WallIndex].DirectionAngle, 0.0);
		EXPECT_EQ(Gap.Extension->Shape, EExtensionShape::Beveled);
		ExpectNear(Gap.Extension->Start, {0.0, 150.0 * Side});
		ExpectNear(*Gap.Corner, {-Reach, 80.0 * Side});
	}
}

TEST(Junction, WallsInLineWithEqualWidthsNeedNoExtension)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(180.0, 100.0, 100.0);

	const Junction Result = Joint.Build();
	ASSERT_TRUE(Result.Gaps[0].Corner);
	ExpectNear(*Result.Gaps[0].Corner, {0.0, 100.0});
	EXPECT_FALSE(Result.Gaps[0].Extension);
	ExpectNear(*Result.Gaps[1].Corner, {0.0, -100.0});
	EXPECT_FALSE(Result.Gaps[1].Extension);
}

TEST(Junction, ExtensionStopsAtMiddleOfShortThinWall)
{
	// The straight extension would meet the thin side about 104 along it, past the middle of a 150 long wall.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 20.0, 20.0, 150.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	ASSERT_TRUE(Gap.Extension);
	EXPECT_EQ(Gap.Extension->Shape, EExtensionShape::ToSideMiddle);
	ExpectNear(*Gap.Corner, -PerpLeft(Direction(120.0)) * 20.0 + Direction(120.0) * 75.0);
}

TEST(Junction, ObtuseCornerExtendsTheSecondWallWhenItIsWider)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 20.0, 20.0);
	Joint.AddIncoming(120.0, 100.0, 100.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	ASSERT_TRUE(Gap.Corner);
	ExpectNear(*Gap.Corner, {(100.0 - 10.0) / std::sin(DegreesToRadians(120.0)), 20.0});

	ASSERT_TRUE(Gap.Extension);
	EXPECT_EQ(Gap.Extension->WallIndex, 1u);
	ExpectNear(Gap.Extension->Start, Direction(30.0) * 100.0);
}

TEST(Junction, ObtuseCornerExactlyAtStepNeedsNoExtension)
{
	// 100 * -cos(120) = 50: the thinner side meets the wider one right at its end.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(120.0, 50.0, 50.0);

	const JunctionGap Gap = Joint.Build().Gaps[0];
	ExpectNear(*Gap.Corner, {0.0, 100.0});
	EXPECT_FALSE(Gap.Extension);
}

TEST(Junction, AcuteAndRightCornersNeverNeedExtension)
{
	for (const double Degrees : {30.0, 60.0, 90.0})
	{
		Star Joint;
		Joint.AddOutgoing(0.0, 300.0, 300.0);
		Joint.AddOutgoing(Degrees, 5.0, 5.0);
		SCOPED_TRACE(Degrees);

		const JunctionGap Gap = Joint.Build().Gaps[0];
		EXPECT_TRUE(Gap.Corner);
		EXPECT_FALSE(Gap.Extension);
	}
}

TEST(Junction, ArcsHaveNoCornerYet)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.Network.AddWall(Joint.Center, Joint.Network.AddNode({500.0, 1000.0}), 100.0, 100.0, 0.2);

	const Junction Result = Joint.Build();
	ASSERT_EQ(Result.Gaps.size(), 2u);
	EXPECT_FALSE(Result.Gaps[0].Corner);
	EXPECT_FALSE(Result.Gaps[1].Corner);
}
