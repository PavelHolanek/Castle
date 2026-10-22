#include "CastleCore/WallLimits.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

using namespace Castle::Core;

namespace
{
	bool Contains(const std::vector<ELimitViolation>& Violations, ELimitViolation Violation)
	{
		return std::find(Violations.begin(), Violations.end(), Violation) != Violations.end();
	}

	std::vector<ELimitViolation> CheckSingleWall(Vec2 Start, Vec2 End, double WidthLeft, double WidthRight, double Bulge = 0.0)
	{
		WallNetwork Network;
		const WallId Wall = Network.AddWall(Network.AddNode(Start), Network.AddNode(End), WidthLeft, WidthRight, Bulge);
		return CheckWall(Network, Wall, WallLimits{});
	}

	/** Two straight walls of length 500 leaving Center at the given angles (degrees). */
	struct TwoWallJoint
	{
		WallNetwork Network;
		NodeId Center;

		TwoWallJoint(double FirstAngle, double SecondAngle, bool bFirstEndsAtCenter = false)
		{
			Center = Network.AddNode({0.0, 0.0});
			const NodeId First = Network.AddNode(PointOnCircle({0.0, 0.0}, 500.0, DegreesToRadians(FirstAngle)));
			const NodeId Second = Network.AddNode(PointOnCircle({0.0, 0.0}, 500.0, DegreesToRadians(SecondAngle)));
			if (bFirstEndsAtCenter)
			{
				Network.AddWall(First, Center, 100.0, 100.0);
			}
			else
			{
				Network.AddWall(Center, First, 100.0, 100.0);
			}
			Network.AddWall(Center, Second, 100.0, 100.0);
		}
	};
}

TEST(WallLimits, StraightWallWithinLimits)
{
	EXPECT_TRUE(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 100.0, 100.0).empty());
}

TEST(WallLimits, Length)
{
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {50.0, 0.0}, 100.0, 100.0), ELimitViolation::TooShort));
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {6000.0, 0.0}, 100.0, 100.0), ELimitViolation::TooLong));
}

TEST(WallLimits, WidthIsTheSumOfBothSides)
{
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 15.0, 15.0), ELimitViolation::TooThin));
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 300.0, 300.0), ELimitViolation::TooThick));
	EXPECT_TRUE(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 0.0, 300.0).empty());
}

TEST(WallLimits, ArcRadiusAndAngle)
{
	// Half circle of radius 100.
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {200.0, 0.0}, 20.0, 20.0, 1.0), ELimitViolation::ArcRadiusTooSmall));

	// 4 * atan(2) is about 253 degrees.
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 50.0, 50.0, 2.0), ELimitViolation::ArcAngleTooLarge));

	// 4 * atan(0.02) is about 4.6 degrees, with a radius of about 125 m.
	const std::vector<ELimitViolation> Flat = CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 50.0, 50.0, 0.02);
	EXPECT_TRUE(Contains(Flat, ELimitViolation::ArcAngleTooSmall));
	EXPECT_TRUE(Contains(Flat, ELimitViolation::ArcRadiusTooLarge));

	// Half circle of radius 500.
	EXPECT_TRUE(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 100.0, 100.0, 1.0).empty());
}

TEST(WallLimits, InnerSideOfArcCollapses)
{
	// Counterclockwise half circle of radius 500, all 500 of the width on the inner (left) side.
	EXPECT_TRUE(Contains(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 500.0, 0.0, 1.0), ELimitViolation::SideCollapses));
	EXPECT_FALSE(Contains(CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 0.0, 500.0, 1.0), ELimitViolation::SideCollapses));
}

TEST(WallLimits, StraightWallIgnoresArcLimits)
{
	const std::vector<ELimitViolation> Violations = CheckSingleWall({0.0, 0.0}, {1000.0, 0.0}, 100.0, 100.0, 0.0);
	EXPECT_FALSE(Contains(Violations, ELimitViolation::ArcRadiusTooLarge));
	EXPECT_FALSE(Contains(Violations, ELimitViolation::ArcAngleTooSmall));
}

TEST(WallLimits, SmallestJointAngle)
{
	EXPECT_NEAR(*GetSmallestJointAngle(TwoWallJoint(0.0, 60.0).Network, NodeId{0}), DegreesToRadians(60.0), 1e-9);
	EXPECT_NEAR(*GetSmallestJointAngle(TwoWallJoint(0.0, 180.0).Network, NodeId{0}), Pi, 1e-9);

	// Wall direction doesn't matter, only the angle between the walls at the node.
	EXPECT_NEAR(*GetSmallestJointAngle(TwoWallJoint(10.0, 300.0, true).Network, NodeId{0}), DegreesToRadians(70.0), 1e-9);
}

TEST(WallLimits, SmallestJointAngleOfThreeWalls)
{
	TwoWallJoint Joint(20.0, 145.0);
	const NodeId Third = Joint.Network.AddNode(PointOnCircle({0.0, 0.0}, 500.0, DegreesToRadians(250.0)));
	Joint.Network.AddWall(Third, Joint.Center, 100.0, 100.0);

	EXPECT_NEAR(*GetSmallestJointAngle(Joint.Network, Joint.Center), DegreesToRadians(105.0), 1e-9);
}

TEST(WallLimits, JointAngleUsesArcTangents)
{
	// Two half circles forming a round tower meet head-on at both nodes.
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({1000.0, 0.0});
	Network.AddWall(A, B, 100.0, 100.0, 1.0);
	Network.AddWall(B, A, 100.0, 100.0, 1.0);

	EXPECT_NEAR(*GetSmallestJointAngle(Network, A), Pi, 1e-9);
	EXPECT_NEAR(*GetSmallestJointAngle(Network, B), Pi, 1e-9);
}

TEST(WallLimits, SharpJoint)
{
	const TwoWallJoint Sharp(0.0, 20.0);
	EXPECT_TRUE(Contains(CheckNode(Sharp.Network, Sharp.Center, WallLimits{}), ELimitViolation::JointTooSharp));

	const TwoWallJoint Acute(0.0, 60.0);
	EXPECT_TRUE(CheckNode(Acute.Network, Acute.Center, WallLimits{}).empty());
}

TEST(WallLimits, NodeWithOneWallHasNoJointAngle)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	Network.AddWall(A, Network.AddNode({1000.0, 0.0}), 100.0, 100.0);

	EXPECT_FALSE(GetSmallestJointAngle(Network, A));
	EXPECT_TRUE(CheckNode(Network, A, WallLimits{}).empty());
}
