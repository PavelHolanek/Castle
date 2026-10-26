#include "CastleCore/Polygon.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

using namespace Castle::Core;

namespace
{
	constexpr double Tolerance = 1e-9;

	std::vector<Vec2> MakeSquare(Vec2 Min, double Size)
	{
		return {Min, {Min.X + Size, Min.Y}, {Min.X + Size, Min.Y + Size}, {Min.X, Min.Y + Size}};
	}
}

TEST(Polygon, SignedAreaFollowsOrientation)
{
	std::vector<Vec2> Square = MakeSquare({0.0, 0.0}, 10.0);
	EXPECT_NEAR(GetSignedArea(Square), 100.0, Tolerance);

	std::reverse(Square.begin(), Square.end());
	EXPECT_NEAR(GetSignedArea(Square), -100.0, Tolerance);
}

TEST(Polygon, RegularPolygonApproachesTheCircle)
{
	const std::vector<Vec2> Hexagon = MakeRegularPolygon({5.0, 5.0}, 2.0, 6);
	ASSERT_EQ(Hexagon.size(), 6u);
	EXPECT_NEAR(Hexagon[0].X, 7.0, Tolerance);
	EXPECT_NEAR(Hexagon[0].Y, 5.0, Tolerance);
	EXPECT_NEAR(GetSignedArea(Hexagon), 1.5 * std::sqrt(3.0) * 4.0, Tolerance);

	EXPECT_NEAR(GetSignedArea(MakeRegularPolygon({0.0, 0.0}, 1.0, 360)), Pi, 1e-3);
}

TEST(Polygon, SimplifyLeavesAPlainPolygonAsItIs)
{
	std::vector<Vec2> Square = MakeSquare({0.0, 0.0}, 10.0);
	SimplifyPolygon(Square);
	EXPECT_EQ(Square, MakeSquare({0.0, 0.0}, 10.0));
}

TEST(Polygon, SimplifyRemovesRepeatedAndCollinearPoints)
{
	std::vector<Vec2> Square = {{0.0, 0.0}, {0.0, 0.0}, {10.0, 0.0}, {10.0, 4.0}, {10.0, 10.0}, {0.0, 10.0}};
	SimplifyPolygon(Square);
	EXPECT_EQ(Square, MakeSquare({0.0, 0.0}, 10.0));
}

TEST(Polygon, SimplifyRemovesASpike)
{
	// Out from the bottom edge and back along the same line.
	std::vector<Vec2> Square = {{0.0, 0.0}, {5.0, 0.0}, {5.0, -3.0}, {5.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
	SimplifyPolygon(Square);
	EXPECT_EQ(Square, MakeSquare({0.0, 0.0}, 10.0));
}

TEST(Polygon, SimplifyOfNoAreaLeavesTooFewPoints)
{
	std::vector<Vec2> Line = {{0.0, 0.0}, {5.0, 0.0}, {10.0, 0.0}};
	SimplifyPolygon(Line);
	EXPECT_LT(Line.size(), 3u);
}

TEST(Polygon, ClipKeepsTheOverlap)
{
	const std::vector<Vec2> Clipped = ClipToConvex(MakeSquare({0.0, 0.0}, 10.0), MakeSquare({5.0, 5.0}, 10.0));
	EXPECT_NEAR(GetSignedArea(Clipped), 25.0, Tolerance);
}

TEST(Polygon, ClipLeavesASubjectInsideAsItIs)
{
	const std::vector<Vec2> Subject = MakeSquare({2.0, 2.0}, 3.0);
	const std::vector<Vec2> Clipped = ClipToConvex(Subject, MakeSquare({0.0, 0.0}, 10.0));
	ASSERT_EQ(Clipped.size(), 4u);
	EXPECT_NEAR(GetSignedArea(Clipped), 9.0, Tolerance);
}

TEST(Polygon, ClipOfSeparatePolygonsIsEmpty)
{
	EXPECT_TRUE(ClipToConvex(MakeSquare({0.0, 0.0}, 1.0), MakeSquare({5.0, 5.0}, 1.0)).empty());
}

TEST(Polygon, ClipCutsTheTipsOffALongKite)
{
	// A long thin kite through a circle keeps only its middle.
	const std::vector<Vec2> Kite = {{-100.0, 0.0}, {0.0, -1.0}, {100.0, 0.0}, {0.0, 1.0}};
	const std::vector<Vec2> Clipped = ClipToConvex(Kite, MakeRegularPolygon({0.0, 0.0}, 10.0, 64));

	EXPECT_GT(GetSignedArea(Clipped), 0.0);
	EXPECT_LT(GetSignedArea(Clipped), 0.2 * GetSignedArea(Kite));
	for (const Vec2 Point : Clipped)
	{
		EXPECT_LE(Length(Point), 10.0 + Tolerance);
	}
}

TEST(Polygon, ClipOfAConcaveSubject)
{
	// An L of area 3 clipped to its lower row keeps area 2.
	const std::vector<Vec2> LShape = {{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {1.0, 1.0}, {1.0, 2.0}, {0.0, 2.0}};
	const std::vector<Vec2> Row = {{-1.0, -1.0}, {3.0, -1.0}, {3.0, 1.0}, {-1.0, 1.0}};
	EXPECT_NEAR(GetSignedArea(ClipToConvex(LShape, Row)), 2.0, Tolerance);
}
