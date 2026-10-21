#include "CastleCore/WallOutline.h"

#include <gtest/gtest.h>

using namespace Castle::Core;

namespace
{
	constexpr double Tolerance = 1e-9;

	void ExpectNear(Vec2 Actual, Vec2 Expected)
	{
		EXPECT_NEAR(Actual.X, Expected.X, Tolerance);
		EXPECT_NEAR(Actual.Y, Expected.Y, Tolerance);
	}

	RawWallOutline BuildSingleWall(Vec2 Start, Vec2 End, double WidthLeft, double WidthRight, double Bulge = 0.0)
	{
		WallNetwork Network;
		const WallId Wall = Network.AddWall(Network.AddNode(Start), Network.AddNode(End), WidthLeft, WidthRight, Bulge);
		return *BuildRawOutline(Network, Wall);
	}
}

TEST(WallOutline, StraightWallAroundCenteredReference)
{
	const RawWallOutline Outline = BuildSingleWall({0.0, 0.0}, {1000.0, 0.0}, 50.0, 50.0);

	ASSERT_TRUE(Outline.Left);
	ASSERT_TRUE(Outline.Right);
	ExpectNear(Outline.Left->GetStart(), {0.0, 50.0});
	ExpectNear(Outline.Left->GetEnd(), {1000.0, 50.0});
	ExpectNear(Outline.Right->GetStart(), {0.0, -50.0});
	ExpectNear(Outline.Right->GetEnd(), {1000.0, -50.0});

	ExpectNear(Outline.StartCap.GetStart(), {0.0, -50.0});
	ExpectNear(Outline.StartCap.GetEnd(), {0.0, 50.0});
	ExpectNear(Outline.EndCap.GetStart(), {1000.0, -50.0});
	ExpectNear(Outline.EndCap.GetEnd(), {1000.0, 50.0});
}

TEST(WallOutline, ReferenceAlongOneFace)
{
	const RawWallOutline Outline = BuildSingleWall({0.0, 0.0}, {1000.0, 0.0}, 0.0, 200.0);

	ExpectNear(Outline.Left->GetStart(), {0.0, 0.0});
	ExpectNear(Outline.Right->GetStart(), {0.0, -200.0});
}

TEST(WallOutline, SidesFollowWallDirection)
{
	// Running towards -X, the left side is at -Y.
	const RawWallOutline Outline = BuildSingleWall({1000.0, 0.0}, {0.0, 0.0}, 100.0, 20.0);

	ExpectNear(Outline.Left->GetStart(), {1000.0, -100.0});
	ExpectNear(Outline.Right->GetStart(), {1000.0, 20.0});
}

TEST(WallOutline, ArcSidesAreConcentricAndMeetTheCaps)
{
	// Counterclockwise half circle of radius 500: the left side is the inner one.
	const RawWallOutline Outline = BuildSingleWall({0.0, 0.0}, {1000.0, 0.0}, 100.0, 60.0, 1.0);

	ASSERT_TRUE(Outline.Left);
	ASSERT_TRUE(Outline.Right);
	ExpectNear(Outline.Left->GetCenter(), Outline.Reference.GetCenter());
	EXPECT_NEAR(Outline.Left->GetRadius(), 400.0, Tolerance);
	EXPECT_NEAR(Outline.Right->GetRadius(), 560.0, Tolerance);

	ExpectNear(Outline.StartCap.GetStart(), Outline.Right->GetStart());
	ExpectNear(Outline.StartCap.GetEnd(), Outline.Left->GetStart());
	ExpectNear(Outline.EndCap.GetStart(), Outline.Right->GetEnd());
	ExpectNear(Outline.EndCap.GetEnd(), Outline.Left->GetEnd());
}

TEST(WallOutline, InnerSideOfTightArcCollapses)
{
	// Half circle of radius 100 with 150 to the inner side.
	const RawWallOutline Outline = BuildSingleWall({0.0, 0.0}, {200.0, 0.0}, 150.0, 50.0, 1.0);

	EXPECT_FALSE(Outline.Left);
	EXPECT_TRUE(Outline.Right);
}

TEST(WallOutline, UnknownWallHasNoOutline)
{
	const WallNetwork Network;

	EXPECT_FALSE(BuildRawOutline(Network, WallId{}));
}
