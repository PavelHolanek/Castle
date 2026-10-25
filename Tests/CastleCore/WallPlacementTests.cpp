#include "CastleCore/WallPlacement.h"

#include <gtest/gtest.h>

#include <algorithm>

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
	Existing Walls;
	WallPlacement Placement = MakePlacement();

	Placement.Click(Walls.Network, {0.0, 1000.0}, SnapRadius);
	Placement.Click(Walls.Network, {80.0, 0.0}, SnapRadius);

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
