#include "CastleCore/WallCurve.h"

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

TEST(WallCurve, LineOffsetGoesLeftForPositiveDistance)
{
	const WallCurve Line = WallCurve::MakeLine({0.0, 0.0}, {100.0, 0.0});

	const std::optional<WallCurve> Left = Line.Offset(10.0);
	ASSERT_TRUE(Left);
	ExpectNear(Left->GetStart(), {0.0, 10.0});
	ExpectNear(Left->GetEnd(), {100.0, 10.0});

	const std::optional<WallCurve> Right = Line.Offset(-10.0);
	ASSERT_TRUE(Right);
	ExpectNear(Right->GetStart(), {0.0, -10.0});
	ExpectNear(Right->GetEnd(), {100.0, -10.0});
}

TEST(WallCurve, ZeroBulgeIsLine)
{
	const WallCurve Curve = WallCurve::FromBulge({0.0, 0.0}, {100.0, 50.0}, 0.0);

	EXPECT_FALSE(Curve.IsArc());
	ExpectNear(Curve.PointAt(0.5), {50.0, 25.0});
}

TEST(WallCurve, PositiveBulgeOneIsHalfCircleBowingRight)
{
	const WallCurve Arc = WallCurve::FromBulge({0.0, 0.0}, {200.0, 0.0}, 1.0);

	ASSERT_TRUE(Arc.IsArc());
	ExpectNear(Arc.GetCenter(), {100.0, 0.0});
	EXPECT_NEAR(Arc.GetRadius(), 100.0, Tolerance);
	EXPECT_NEAR(Arc.GetSweepAngle(), Pi, Tolerance);
	ExpectNear(Arc.PointAt(0.5), {100.0, -100.0});
	EXPECT_NEAR(Arc.GetLength(), 100.0 * Pi, Tolerance);
}

TEST(WallCurve, NegativeBulgeBowsLeft)
{
	const WallCurve Arc = WallCurve::FromBulge({0.0, 0.0}, {200.0, 0.0}, -1.0);

	EXPECT_NEAR(Arc.GetSweepAngle(), -Pi, Tolerance);
	ExpectNear(Arc.PointAt(0.5), {100.0, 100.0});
}

TEST(WallCurve, QuarterArcFromBulge)
{
	const WallCurve Arc = WallCurve::FromBulge({100.0, 0.0}, {0.0, 100.0}, std::tan(Pi / 8.0));

	ExpectNear(Arc.GetCenter(), {0.0, 0.0});
	EXPECT_NEAR(Arc.GetRadius(), 100.0, Tolerance);
	EXPECT_NEAR(Arc.GetSweepAngle(), Pi / 2.0, Tolerance);
	ExpectNear(Arc.TangentAt(0.0), {0.0, 1.0});
	ExpectNear(Arc.TangentAt(1.0), {-1.0, 0.0});
}

TEST(WallCurve, ArcPassesThroughBothEnds)
{
	const Vec2 Start{-30.0, 40.0};
	const Vec2 End{250.0, 120.0};

	for (const double Bulge : {-3.0, -1.0, -0.4, 0.1, 0.5, 1.0, 2.5})
	{
		const WallCurve Arc = WallCurve::FromBulge(Start, End, Bulge);
		SCOPED_TRACE(Bulge);
		ExpectNear(Arc.GetStart(), Start);
		ExpectNear(Arc.GetEnd(), End);
		EXPECT_NEAR(Arc.GetSweepAngle(), 4.0 * std::atan(Bulge), Tolerance);
	}
}

TEST(WallCurve, ArcOffsetIsConcentric)
{
	const WallCurve Ccw = WallCurve::MakeArc({0.0, 0.0}, 100.0, 0.0, Pi / 2.0);
	const WallCurve Cw = WallCurve::MakeArc({0.0, 0.0}, 100.0, Pi / 2.0, -Pi / 2.0);

	// The center is on the left of a counterclockwise arc and on the right of a clockwise one.
	EXPECT_NEAR(Ccw.Offset(10.0)->GetRadius(), 90.0, Tolerance);
	EXPECT_NEAR(Ccw.Offset(-10.0)->GetRadius(), 110.0, Tolerance);
	EXPECT_NEAR(Cw.Offset(10.0)->GetRadius(), 110.0, Tolerance);
	EXPECT_NEAR(Cw.Offset(-10.0)->GetRadius(), 90.0, Tolerance);

	ExpectNear(Ccw.Offset(10.0)->GetCenter(), Ccw.GetCenter());
	EXPECT_NEAR(Ccw.Offset(10.0)->GetSweepAngle(), Ccw.GetSweepAngle(), Tolerance);
}

TEST(WallCurve, LeftOffsetLiesLeftOfTangent)
{
	for (const double Bulge : {-0.8, -0.2, 0.0, 0.3, 1.5})
	{
		const WallCurve Curve = WallCurve::FromBulge({0.0, 0.0}, {500.0, 200.0}, Bulge);
		const std::optional<WallCurve> Left = Curve.Offset(25.0);
		ASSERT_TRUE(Left);
		SCOPED_TRACE(Bulge);

		for (const double T : {0.0, 0.25, 0.5, 1.0})
		{
			const Vec2 Point = Curve.PointAt(T);
			const Vec2 Shift = Left->PointAt(T) - Point;
			EXPECT_NEAR(Length(Shift), 25.0, 1e-6);
			EXPECT_GT(Cross(Curve.TangentAt(T), Shift), 0.0);
		}
	}
}

TEST(WallCurve, ArcOffsetCollapsesAtRadius)
{
	const WallCurve Arc = WallCurve::MakeArc({0.0, 0.0}, 100.0, 0.0, Pi / 2.0);

	EXPECT_FALSE(Arc.Offset(100.0));
	EXPECT_FALSE(Arc.Offset(150.0));
	EXPECT_TRUE(Arc.Offset(-150.0));
}

TEST(WallCurve, LineTessellatesToItsEnds)
{
	const WallCurve Line = WallCurve::MakeLine({1.0, 2.0}, {3.0, 4.0});

	const std::vector<Vec2> Points = Line.Tessellate(0.1);
	ASSERT_EQ(Points.size(), 2u);
	ExpectNear(Points[0], {1.0, 2.0});
	ExpectNear(Points[1], {3.0, 4.0});
}

TEST(WallCurve, ArcTessellationRespectsAngleStep)
{
	const WallCurve Arc = WallCurve::MakeArc({10.0, 20.0}, 50.0, 0.3, Pi);

	EXPECT_EQ(Arc.Tessellate(Pi / 4.0).size(), 5u);

	// Pi / 0.3 is not a whole number: the pieces get smaller, never larger than the step.
	const std::vector<Vec2> Points = Arc.Tessellate(0.3);
	ASSERT_EQ(Points.size(), 12u);
	ExpectNear(Points.front(), Arc.GetStart());
	ExpectNear(Points.back(), Arc.GetEnd());
	for (const Vec2& Point : Points)
	{
		EXPECT_NEAR(Distance(Point, Arc.GetCenter()), 50.0, Tolerance);
	}
}
