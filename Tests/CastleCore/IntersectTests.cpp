#include "CastleCore/Intersect.h"

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
}

TEST(Intersect, LinesCross)
{
	const std::optional<Vec2> Crossing = IntersectLines({0.0, 0.0}, {1.0, 0.0}, {3.0, 5.0}, {0.0, 1.0});
	ASSERT_TRUE(Crossing);
	ExpectNear(*Crossing, {3.0, 0.0});
}

TEST(Intersect, LinesCrossBehindTheirPoints)
{
	// Lines, not rays: the crossing may lie behind either point.
	const Vec2 Diagonal{std::sqrt(0.5), std::sqrt(0.5)};
	const std::optional<Vec2> Crossing = IntersectLines({10.0, 10.0}, Diagonal, {-4.0, 2.0}, {0.0, 1.0});
	ASSERT_TRUE(Crossing);
	ExpectNear(*Crossing, {-4.0, -4.0});
}

TEST(Intersect, ParallelLinesDontCross)
{
	EXPECT_FALSE(IntersectLines({0.0, 0.0}, {1.0, 0.0}, {0.0, 5.0}, {-1.0, 0.0}));

	// The same line has no single crossing either.
	EXPECT_FALSE(IntersectLines({0.0, 0.0}, {1.0, 0.0}, {7.0, 0.0}, {1.0, 0.0}));
}

TEST(Intersect, RayFromInsideLeavesTheCircle)
{
	const std::optional<Vec2> FromCenter = IntersectRayWithCircle({0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, 5.0);
	ASSERT_TRUE(FromCenter);
	ExpectNear(*FromCenter, {5.0, 0.0});

	const std::optional<Vec2> OffCenter = IntersectRayWithCircle({3.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}, 5.0);
	ASSERT_TRUE(OffCenter);
	ExpectNear(*OffCenter, {3.0, 4.0});
}

TEST(Intersect, RayFromOutsideGivesTheFartherCrossing)
{
	const std::optional<Vec2> Crossing = IntersectRayWithCircle({-10.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, 5.0);
	ASSERT_TRUE(Crossing);
	ExpectNear(*Crossing, {5.0, 0.0});
}

TEST(Intersect, RayMissingOrLeavingTheCircleBehind)
{
	EXPECT_FALSE(IntersectRayWithCircle({-10.0, 6.0}, {1.0, 0.0}, {0.0, 0.0}, 5.0));
	EXPECT_FALSE(IntersectRayWithCircle({10.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, 5.0));
}
