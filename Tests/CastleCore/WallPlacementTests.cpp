#include "CastleCore/WallPlacement.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <utility>

using namespace Castle::Core;

namespace
{
	WallPlacement MakePlacement()
	{
		WallPlacement Placement;
		Placement.SetWidths(75.0, 75.0);
		return Placement;
	}

	bool Contains(const std::vector<ELimitViolation>& Violations, ELimitViolation Violation)
	{
		return std::find(Violations.begin(), Violations.end(), Violation) != Violations.end();
	}
}

TEST(WallPlacement, StartsWithoutPreview)
{
	const WallPlacement Placement = MakePlacement();

	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingStart);
	EXPECT_FALSE(Placement.GetPreview());
}

TEST(WallPlacement, HoverBeforeFirstClickShowsNothing)
{
	WallPlacement Placement = MakePlacement();
	const WallNetwork Network;

	Placement.Hover(Network, {100.0, 0.0});

	EXPECT_FALSE(Placement.GetPreview());
}

TEST(WallPlacement, PreviewFollowsCursorWithoutTouchingNetwork)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	EXPECT_EQ(Placement.Click(Network, {0.0, 0.0}), WallPlacement::EClickResult::Started);
	Placement.Hover(Network, {500.0, 0.0});
	Placement.Hover(Network, {0.0, 800.0});

	ASSERT_TRUE(Placement.GetPreview());
	const WallNetwork& Preview = *Placement.GetPreview();
	EXPECT_EQ(Preview.GetWallCount(), 1u);

	const Wall* Placed = Preview.FindWall(Placement.GetPreviewWall());
	ASSERT_NE(Placed, nullptr);
	EXPECT_EQ(Placed->WidthLeft, 75.0);
	EXPECT_EQ(Placed->WidthRight, 75.0);
	EXPECT_TRUE(IsNearlyEqual(Preview.FindNode(Placed->NodeA)->Position, {0.0, 0.0}));
	EXPECT_TRUE(IsNearlyEqual(Preview.FindNode(Placed->NodeB)->Position, {0.0, 800.0}));
	EXPECT_TRUE(Placement.GetPreviewViolations().empty());

	EXPECT_EQ(Network.GetWallCount(), 0u);
	EXPECT_EQ(Network.GetNodeCount(), 0u);
}

TEST(WallPlacement, SecondClickCommitsTheWall)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});
	Placement.Hover(Network, {400.0, 0.0});
	const WallId Previewed = Placement.GetPreviewWall();

	EXPECT_EQ(Placement.Click(Network, {500.0, 0.0}), WallPlacement::EClickResult::Committed);

	EXPECT_EQ(Network.GetWallCount(), 1u);
	EXPECT_EQ(Network.GetNodeCount(), 2u);
	const std::optional<WallCurve> Reference = Network.GetReferenceCurve(Previewed);
	ASSERT_TRUE(Reference);
	EXPECT_TRUE(IsNearlyEqual(Reference->GetEnd(), {500.0, 0.0}));

	// The chain goes on from the wall's end.
	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingEnd);
}

TEST(WallPlacement, ChainContinuesFromTheEndOfTheLastWall)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});
	Placement.Click(Network, {500.0, 0.0});
	const NodeId Corner = Network.FindWall(WallId{0})->NodeB;

	Placement.Hover(Network, {500.0, 500.0});
	EXPECT_EQ(Placement.GetPreview()->FindWall(Placement.GetPreviewWall())->NodeA, Corner);

	EXPECT_EQ(Placement.Click(Network, {500.0, 500.0}), WallPlacement::EClickResult::Committed);
	EXPECT_EQ(Network.GetWallCount(), 2u);
	EXPECT_EQ(Network.GetNodeCount(), 3u);
	EXPECT_EQ(Network.GetNodeWalls(Corner).size(), 2u);
}

TEST(WallPlacement, CancelEndsTheChain)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});
	Placement.Click(Network, {500.0, 0.0});
	Placement.Cancel();

	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingStart);
	EXPECT_EQ(Placement.Click(Network, {0.0, 500.0}), WallPlacement::EClickResult::Started);
	Placement.Click(Network, {500.0, 500.0});
	EXPECT_EQ(Network.GetNodeCount(), 4u);
}

TEST(WallPlacement, WallOutsideLimitsIsRejected)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});

	EXPECT_EQ(Placement.Click(Network, {50.0, 0.0}), WallPlacement::EClickResult::Rejected);
	EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::TooShort));
	EXPECT_EQ(Network.GetWallCount(), 0u);

	// Still placing: a valid end commits.
	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingEnd);
	EXPECT_EQ(Placement.Click(Network, {500.0, 0.0}), WallPlacement::EClickResult::Committed);
}

TEST(WallPlacement, WidthsOutsideLimitsAreRejected)
{
	WallPlacement Placement;
	Placement.SetWidths(10.0, 10.0);
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});

	EXPECT_EQ(Placement.Click(Network, {500.0, 0.0}), WallPlacement::EClickResult::Rejected);
	EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::TooThin));
}

TEST(WallPlacement, CancelDropsTheWall)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});
	Placement.Hover(Network, {500.0, 0.0});
	Placement.Cancel();

	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingStart);
	EXPECT_FALSE(Placement.GetPreview());
	EXPECT_EQ(Network.GetWallCount(), 0u);
}

TEST(WallPlacement, PreviewKeepsExistingWalls)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;
	Network.AddWall(Network.AddNode({0.0, 1000.0}), Network.AddNode({1000.0, 1000.0}), 50.0, 50.0);

	Placement.Click(Network, {0.0, 0.0});
	Placement.Hover(Network, {1000.0, 0.0});
	EXPECT_EQ(Placement.GetPreview()->GetWallCount(), 2u);

	Placement.Click(Network, {1000.0, 0.0});
	EXPECT_EQ(Network.GetWallCount(), 2u);
	EXPECT_EQ(Network.GetNodeCount(), 4u);
}

// Snapping

namespace
{
	/** A single wall from A (0, 0) to B (1000, 0). */
	struct Existing
	{
		WallNetwork Network;
		NodeId A = Network.AddNode({0.0, 0.0});
		NodeId B = Network.AddNode({1000.0, 0.0});
		WallId AB = Network.AddWall(A, B, 50.0, 50.0);
	};

	constexpr double SnapRadius = 20.0;
}

TEST(WallPlacement, HoverReportsTheNodeItWouldSnapTo)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Hover(Walls.Network, {1010.0, 5.0}, SnapRadius);
	EXPECT_EQ(Placement.GetSnapNode(), Walls.B);

	// The existing wall is 100 wide, so it snaps within 75.
	Placement.Hover(Walls.Network, {1080.0, 5.0}, SnapRadius);
	EXPECT_FALSE(Placement.GetSnapNode().IsValid());
}

TEST(WallPlacement, StartSnapsToExistingNode)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {1005.0, 3.0}, SnapRadius);
	Placement.Hover(Walls.Network, {1000.0, 800.0}, SnapRadius);

	const WallNetwork& Preview = *Placement.GetPreview();
	EXPECT_EQ(Preview.FindWall(Placement.GetPreviewWall())->NodeA, Walls.B);
	EXPECT_EQ(Preview.GetNodeCount(), 3u);
}

TEST(WallPlacement, EndSnapsToExistingNode)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {0.0, 1000.0}, SnapRadius);
	EXPECT_EQ(Placement.Click(Walls.Network, {5.0, -5.0}, SnapRadius), WallPlacement::EClickResult::Committed);

	EXPECT_EQ(Walls.Network.GetNodeCount(), 3u);
	EXPECT_EQ(Walls.Network.GetNodeWalls(Walls.A).size(), 2u);
	EXPECT_TRUE(IsNearlyEqual(Walls.Network.FindNode(Walls.A)->Position, {0.0, 0.0}));

	// Joining an existing node ends the chain.
	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingStart);
	EXPECT_FALSE(Placement.GetPreview());
}

TEST(WallPlacement, ChainGoesOnPastNodesItDoesNotSnapTo)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {0.0, 1000.0}, SnapRadius);
	Placement.Click(Walls.Network, {500.0, 1000.0}, SnapRadius);

	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingEnd);
}

TEST(WallPlacement, NodeOutsideSnapRadiusIsNotUsed)
{
	// 100 from A, and off the wall.
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {0.0, -1000.0}, SnapRadius);
	Placement.Click(Walls.Network, {80.0, -60.0}, SnapRadius);

	EXPECT_EQ(Walls.Network.GetNodeCount(), 4u);
	EXPECT_EQ(Walls.Network.GetNodeWalls(Walls.A).size(), 1u);
}

TEST(WallPlacement, ChainCanCloseALoop)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0}, SnapRadius);
	Placement.Click(Network, {1000.0, 0.0}, SnapRadius);
	Placement.Click(Network, {1000.0, 1000.0}, SnapRadius);
	EXPECT_EQ(Placement.Click(Network, {8.0, 8.0}, SnapRadius), WallPlacement::EClickResult::Committed);

	EXPECT_EQ(Network.GetNodeCount(), 3u);
	EXPECT_EQ(Network.GetWallCount(), 3u);
	for (const NodeId Node : Network.GetNodeIds())
	{
		EXPECT_EQ(Network.GetNodeWalls(Node).size(), 2u);
	}
	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingStart);
}

TEST(WallPlacement, EndDoesNotSnapToItsOwnStart)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {1000.0, 0.0}, SnapRadius);
	Placement.Hover(Walls.Network, {1005.0, 0.0}, SnapRadius);

	EXPECT_FALSE(Placement.GetSnapNode().IsValid());
	EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::TooShort));
	EXPECT_EQ(Placement.Click(Walls.Network, {1005.0, 0.0}, SnapRadius), WallPlacement::EClickResult::Rejected);
}

TEST(WallPlacement, TooSharpJointWithExistingWallIsRejected)
{
	// From B back towards A, about 6 degrees off the existing wall.
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {1000.0, 0.0}, SnapRadius);

	EXPECT_EQ(Placement.Click(Walls.Network, {0.0, 100.0}, SnapRadius), WallPlacement::EClickResult::Rejected);
	EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::JointTooSharp));
}

TEST(WallPlacement, SecondWallBetweenTheSameNodesIsRejected)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {0.0, 0.0}, SnapRadius);

	EXPECT_EQ(Placement.Click(Walls.Network, {1000.0, 0.0}, SnapRadius), WallPlacement::EClickResult::Rejected);
	EXPECT_EQ(Walls.Network.GetWallCount(), 1u);
}

TEST(WallPlacement, StartNodeRemovedMeanwhileIsRecreated)
{
	WallPlacement Placement = MakePlacement();
	WallNetwork Network;

	Placement.Click(Network, {0.0, 0.0});
	Placement.Click(Network, {500.0, 0.0});
	Network = WallNetwork();

	EXPECT_EQ(Placement.Click(Network, {500.0, 500.0}), WallPlacement::EClickResult::Committed);
	EXPECT_EQ(Network.GetNodeCount(), 2u);
	EXPECT_EQ(Network.GetWallCount(), 1u);
}

TEST(WallPlacement, SnapRadiusGrowsWithTheWidestWallAtTheNode)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({1000.0, 0.0});
	const NodeId Alone = Network.AddNode({0.0, 2000.0});
	Network.AddWall(A, B, 50.0, 50.0);
	Network.AddWall(A, Network.AddNode({0.0, -1000.0}), 150.0, 150.0);
	WallPlacement Placement = MakePlacement();

	EXPECT_NEAR(Placement.GetSnapRadius(Network, A, SnapRadius), 0.75 * 300.0, 1e-9);
	EXPECT_NEAR(Placement.GetSnapRadius(Network, B, SnapRadius), 0.75 * 100.0, 1e-9);
	EXPECT_NEAR(Placement.GetSnapRadius(Network, Alone, SnapRadius), SnapRadius, 1e-9);

	Placement.Hover(Network, {200.0, 50.0}, SnapRadius);
	EXPECT_EQ(Placement.GetSnapNode(), A);
	Placement.Hover(Network, {240.0, 0.0}, SnapRadius);
	EXPECT_FALSE(Placement.GetSnapNode().IsValid());

	Placement.SetSnapWidthFactor(1.0);
	Placement.Hover(Network, {240.0, 0.0}, SnapRadius);
	EXPECT_EQ(Placement.GetSnapNode(), A);
}

TEST(WallPlacement, ExcludedNodeLeavesTheNextOneToSnapTo)
{
	// Two nodes 50 apart, both within reach; the start of the wall being placed is excluded, the other one is used.
	WallNetwork Network;
	const NodeId Near = Network.AddNode({0.0, 0.0});
	const NodeId Other = Network.AddNode({50.0, 0.0});
	WallPlacement Placement = MakePlacement();

	Placement.Click(Network, {0.0, 0.0}, 100.0);
	Placement.Hover(Network, {10.0, 0.0}, 100.0);
	EXPECT_NE(Placement.GetSnapNode(), Near);
	EXPECT_EQ(Placement.GetSnapNode(), Other);
}

// Snapping to walls

TEST(WallPlacement, HoverOverAWallReportsWhereItWouldSplit)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Hover(Walls.Network, {400.0, 30.0}, SnapRadius);

	EXPECT_FALSE(Placement.GetSnapNode().IsValid());
	ASSERT_TRUE(Placement.GetSnapWall());
	EXPECT_EQ(Placement.GetSnapWall()->Wall, Walls.AB);
	EXPECT_TRUE(IsNearlyEqual(Placement.GetSnapWall()->Position, {400.0, 0.0}));

	Placement.Hover(Walls.Network, {400.0, 60.0}, SnapRadius);
	EXPECT_FALSE(Placement.GetSnapWall());
}

TEST(WallPlacement, NodeWinsOverWall)
{
	// Over the wall as well as within the snap radius of its end.
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Hover(Walls.Network, {960.0, 30.0}, SnapRadius);

	EXPECT_EQ(Placement.GetSnapNode(), Walls.B);
	EXPECT_FALSE(Placement.GetSnapWall());
}

TEST(WallPlacement, StartOnAWallSplitsIt)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {400.0, 30.0}, SnapRadius);
	Placement.Hover(Walls.Network, {400.0, 800.0}, SnapRadius);

	// Only in the preview so far.
	const WallNetwork& Preview = *Placement.GetPreview();
	EXPECT_EQ(Preview.GetWallCount(), 3u);
	EXPECT_EQ(Preview.GetNodeCount(), 4u);
	const NodeId Split = Preview.FindWall(Placement.GetPreviewWall())->NodeA;
	EXPECT_TRUE(IsNearlyEqual(Preview.FindNode(Split)->Position, {400.0, 0.0}));
	EXPECT_EQ(Preview.GetNodeWalls(Split).size(), 3u);
	EXPECT_TRUE(Placement.GetPreviewViolations().empty());
	EXPECT_EQ(Walls.Network.GetWallCount(), 1u);

	EXPECT_EQ(Placement.Click(Walls.Network, {400.0, 800.0}, SnapRadius), WallPlacement::EClickResult::Committed);
	EXPECT_EQ(Walls.Network.GetWallCount(), 3u);
	EXPECT_EQ(Walls.Network.GetNodeWalls(Split).size(), 3u);

	// A free end goes on with the chain.
	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingEnd);
}

TEST(WallPlacement, EndOnAWallSplitsItAndEndsTheChain)
{
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {600.0, 800.0}, SnapRadius);
	EXPECT_EQ(Placement.Click(Walls.Network, {600.0, -40.0}, SnapRadius), WallPlacement::EClickResult::Committed);

	EXPECT_EQ(Walls.Network.GetWallCount(), 3u);
	EXPECT_EQ(Walls.Network.GetNodeCount(), 4u);
	const NodeId Split = Walls.Network.FindNearestNode({600.0, 0.0}, 1e-6);
	ASSERT_TRUE(Split.IsValid());
	EXPECT_EQ(Walls.Network.GetNodeWalls(Split).size(), 3u);
	EXPECT_EQ(Walls.Network.GetNodeWalls(Walls.A).size(), 1u);
	EXPECT_EQ(Walls.Network.GetNodeWalls(Walls.B).size(), 1u);

	EXPECT_EQ(Placement.GetState(), WallPlacement::EState::PickingStart);
}

TEST(WallPlacement, SplitTooCloseToANodeIsRejected)
{
	// The split would be 80 from B, which is outside the node's snap radius but leaves a part below the minimum length.
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {920.0, 800.0}, SnapRadius);

	EXPECT_EQ(Placement.Click(Walls.Network, {920.0, -45.0}, SnapRadius), WallPlacement::EClickResult::Rejected);
	EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::TooShort));
	EXPECT_EQ(Walls.Network.GetWallCount(), 1u);
}

TEST(WallPlacement, TooSharpJointWithASplitWallIsRejected)
{
	// About 13 degrees off the wall.
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {0.0, 200.0}, SnapRadius);

	EXPECT_EQ(Placement.Click(Walls.Network, {850.0, -10.0}, SnapRadius), WallPlacement::EClickResult::Rejected);
	EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::JointTooSharp));
}

TEST(WallPlacement, BothEndsOnTheSameWallSplitItTwice)
{
	// Along the wall, so the joints are too sharp; the parts must come out right nonetheless, in either direction.
	for (const auto& [StartX, EndX] : {std::pair{200.0, 700.0}, std::pair{700.0, 200.0}})
	{
		SCOPED_TRACE(StartX);
		Existing Walls;
		WallPlacement Placement = MakePlacement();

		Placement.Click(Walls.Network, {StartX, 10.0}, SnapRadius);
		Placement.Hover(Walls.Network, {EndX, -10.0}, SnapRadius);

		const WallNetwork& Preview = *Placement.GetPreview();
		EXPECT_EQ(Preview.GetNodeCount(), 4u);
		EXPECT_EQ(Preview.GetWallCount(), 4u);
		EXPECT_TRUE(Contains(Placement.GetPreviewViolations(), ELimitViolation::JointTooSharp));

		const Wall* Placed = Preview.FindWall(Placement.GetPreviewWall());
		EXPECT_TRUE(IsNearlyEqual(Preview.FindNode(Placed->NodeA)->Position, {StartX, 0.0}));
		EXPECT_TRUE(IsNearlyEqual(Preview.FindNode(Placed->NodeB)->Position, {EndX, 0.0}));

		// The three parts run from A to B one after another.
		double PartsLength = 0.0;
		for (const WallId Part : Preview.GetWallIds())
		{
			if (Part != Placement.GetPreviewWall())
			{
				const WallCurve Reference = *Preview.GetReferenceCurve(Part);
				EXPECT_GT(Reference.GetEnd().X, Reference.GetStart().X);
				PartsLength += Reference.GetLength();
			}
		}
		EXPECT_NEAR(PartsLength, 1000.0, 1e-6);
	}
}

TEST(WallPlacement, ChordAcrossAnArc)
{
	// A half circle around (1000, 0) with radius 1000. The chord from a quarter to three quarters of it meets it at 45 degrees.
	WallNetwork Network;
	const WallId Arc = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({2000.0, 0.0}), 50.0, 50.0, 1.0);
	const WallCurve Original = *Network.GetReferenceCurve(Arc);
	WallPlacement Placement = MakePlacement();

	Placement.Click(Network, Original.PointAt(0.25), SnapRadius);
	EXPECT_EQ(Placement.Click(Network, Original.PointAt(0.75), SnapRadius), WallPlacement::EClickResult::Committed);

	EXPECT_EQ(Network.GetNodeCount(), 4u);
	EXPECT_EQ(Network.GetWallCount(), 4u);
	EXPECT_TRUE(Network.FindNearestNode(Original.PointAt(0.25), 1e-6).IsValid());
	EXPECT_TRUE(Network.FindNearestNode(Original.PointAt(0.75), 1e-6).IsValid());

	// The arc's three parts stay on its circle and cover all of it.
	double ArcLength = 0.0;
	for (const WallId Part : Network.GetWallIds())
	{
		const WallCurve Reference = *Network.GetReferenceCurve(Part);
		if (Reference.IsArc())
		{
			EXPECT_TRUE(IsNearlyEqual(Reference.GetCenter(), Original.GetCenter()));
			EXPECT_NEAR(Reference.GetRadius(), 1000.0, 1e-6);
			ArcLength += Reference.GetLength();
		}
	}
	EXPECT_NEAR(ArcLength, Original.GetLength(), 1e-6);
}
