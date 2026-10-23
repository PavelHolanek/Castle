#include "CastleCore/WallNetwork.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace Castle::Core;

namespace
{
	bool Contains(std::span<const WallId> Walls, WallId Id)
	{
		return std::find(Walls.begin(), Walls.end(), Id) != Walls.end();
	}
}

TEST(WallNetwork, WallsAreAttachedToBothNodes)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({100.0, 0.0});
	const NodeId C = Network.AddNode({100.0, 100.0});

	const WallId AB = Network.AddWall(A, B, 50.0, 50.0);
	const WallId BC = Network.AddWall(B, C, 30.0, 30.0);

	ASSERT_TRUE(AB.IsValid());
	ASSERT_TRUE(BC.IsValid());
	EXPECT_EQ(Network.GetNodeCount(), 3u);
	EXPECT_EQ(Network.GetWallCount(), 2u);

	EXPECT_EQ(Network.GetNodeWalls(A).size(), 1u);
	EXPECT_EQ(Network.GetNodeWalls(B).size(), 2u);
	EXPECT_TRUE(Contains(Network.GetNodeWalls(B), AB));
	EXPECT_TRUE(Contains(Network.GetNodeWalls(B), BC));
	EXPECT_TRUE(Contains(Network.GetNodeWalls(C), BC));
}

TEST(WallNetwork, RejectsInvalidWalls)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({100.0, 0.0});

	EXPECT_FALSE(Network.AddWall(A, A, 10.0, 10.0).IsValid());
	EXPECT_FALSE(Network.AddWall(A, NodeId{}, 10.0, 10.0).IsValid());
	EXPECT_FALSE(Network.AddWall(A, NodeId{42}, 10.0, 10.0).IsValid());
	EXPECT_FALSE(Network.AddWall(A, B, -1.0, 10.0).IsValid());
	EXPECT_FALSE(Network.AddWall(A, B, 10.0, -1.0).IsValid());

	EXPECT_EQ(Network.GetWallCount(), 0u);
	EXPECT_TRUE(Network.GetNodeWalls(A).empty());
}

TEST(WallNetwork, TwoArcsMayShareBothNodes)
{
	// A round tower: two half circles between the same pair of nodes.
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({400.0, 0.0});

	EXPECT_TRUE(Network.AddWall(A, B, 80.0, 80.0, 1.0).IsValid());
	EXPECT_TRUE(Network.AddWall(B, A, 80.0, 80.0, 1.0).IsValid());
	EXPECT_EQ(Network.GetNodeWalls(A).size(), 2u);
}

TEST(WallNetwork, MovingNodeChangesReferenceCurve)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({100.0, 0.0});
	const WallId Wall = Network.AddWall(A, B, 10.0, 10.0);

	ASSERT_TRUE(Network.MoveNode(B, {0.0, 300.0}));

	const std::optional<WallCurve> Reference = Network.GetReferenceCurve(Wall);
	ASSERT_TRUE(Reference);
	EXPECT_TRUE(IsNearlyEqual(Reference->GetEnd(), {0.0, 300.0}));
	EXPECT_FALSE(Network.MoveNode(NodeId{}, {1.0, 1.0}));
}

TEST(WallNetwork, CornerStyleIsSharpUnlessSet)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	EXPECT_EQ(Network.FindNode(A)->CornerStyle, ECornerStyle::Sharp);

	ASSERT_TRUE(Network.SetCornerStyle(A, ECornerStyle::Beveled));
	EXPECT_EQ(Network.FindNode(A)->CornerStyle, ECornerStyle::Beveled);
	EXPECT_FALSE(Network.SetCornerStyle(NodeId{}, ECornerStyle::Beveled));
}

TEST(WallNetwork, RemoveWallDetachesItFromNodes)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({100.0, 0.0});
	const WallId Wall = Network.AddWall(A, B, 10.0, 10.0);

	ASSERT_TRUE(Network.RemoveWall(Wall));

	EXPECT_EQ(Network.FindWall(Wall), nullptr);
	EXPECT_EQ(Network.GetWallCount(), 0u);
	EXPECT_TRUE(Network.GetNodeWalls(A).empty());
	EXPECT_TRUE(Network.GetNodeWalls(B).empty());
	EXPECT_FALSE(Network.RemoveWall(Wall));
}

TEST(WallNetwork, RemoveNodeRemovesItsWallsAndKeepsOtherIds)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({100.0, 0.0});
	const NodeId C = Network.AddNode({200.0, 0.0});
	const WallId AB = Network.AddWall(A, B, 10.0, 10.0);
	const WallId BC = Network.AddWall(B, C, 10.0, 10.0);

	ASSERT_TRUE(Network.RemoveNode(B));

	EXPECT_EQ(Network.FindNode(B), nullptr);
	EXPECT_EQ(Network.FindWall(AB), nullptr);
	EXPECT_EQ(Network.FindWall(BC), nullptr);
	EXPECT_EQ(Network.GetNodeCount(), 2u);
	EXPECT_EQ(Network.GetWallCount(), 0u);

	ASSERT_NE(Network.FindNode(C), nullptr);
	EXPECT_TRUE(IsNearlyEqual(Network.FindNode(C)->Position, {200.0, 0.0}));
	EXPECT_TRUE(Network.GetNodeWalls(A).empty());

	// Removed ids are not handed out again.
	const NodeId D = Network.AddNode({300.0, 0.0});
	EXPECT_NE(D, B);
	EXPECT_EQ(Network.GetNodeIds(), (std::vector<NodeId>{A, C, D}));
}

TEST(WallNetwork, UnknownIdsFindNothing)
{
	const WallNetwork Network;

	EXPECT_EQ(Network.FindNode(NodeId{}), nullptr);
	EXPECT_EQ(Network.FindWall(WallId{7}), nullptr);
	EXPECT_TRUE(Network.GetNodeWalls(NodeId{3}).empty());
	EXPECT_FALSE(Network.GetReferenceCurve(WallId{}));
	EXPECT_TRUE(Network.GetWallIds().empty());
}
