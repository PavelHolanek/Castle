#include "CastleCore/WallDemolition.h"

#include <gtest/gtest.h>

using namespace Castle::Core;

namespace
{
	/** A wall from A (0, 0) to B (1000, 0) with a stem from its middle M up to S (500, 600). */
	struct TJunction
	{
		WallNetwork Network;
		NodeId A = Network.AddNode({0.0, 0.0});
		NodeId B = Network.AddNode({1000.0, 0.0});
		WallId AB = Network.AddWall(A, B, 50.0, 50.0);
		NodeId M = Network.SplitWall(AB, 0.5);
		NodeId S = Network.AddNode({500.0, 600.0});
		WallId Stem = Network.AddWall(M, S, 50.0, 50.0);
	};
}

TEST(WallDemolition, NodesLeftWithoutWallsGoToo)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({1000.0, 0.0});
	const NodeId C = Network.AddNode({1000.0, 1000.0});
	const WallId AB = Network.AddWall(A, B, 50.0, 50.0);
	Network.AddWall(B, C, 50.0, 50.0);

	EXPECT_TRUE(DemolishWall(Network, AB, WallLimits{}));

	EXPECT_EQ(Network.GetWallCount(), 1u);
	EXPECT_EQ(Network.FindNode(A), nullptr);
	EXPECT_NE(Network.FindNode(B), nullptr);
	EXPECT_EQ(Network.GetNodeCount(), 2u);
}

TEST(WallDemolition, RemovingTheStemOfATJoinsTheWallAgain)
{
	TJunction T;

	EXPECT_TRUE(DemolishWall(T.Network, T.Stem, WallLimits{}));

	EXPECT_EQ(T.Network.GetNodeCount(), 2u);
	EXPECT_EQ(T.Network.GetWallCount(), 1u);
	const Wall* Joined = T.Network.FindWall(T.AB);
	ASSERT_NE(Joined, nullptr);
	EXPECT_EQ(Joined->NodeA, T.A);
	EXPECT_EQ(Joined->NodeB, T.B);
}

TEST(WallDemolition, RemovingAPartOfTheTKeepsTheCorner)
{
	TJunction T;

	EXPECT_TRUE(DemolishWall(T.Network, T.AB, WallLimits{}));

	EXPECT_EQ(T.Network.FindNode(T.A), nullptr);
	EXPECT_EQ(T.Network.GetNodeCount(), 3u);
	EXPECT_EQ(T.Network.GetWallCount(), 2u);
	EXPECT_EQ(T.Network.GetNodeWalls(T.M).size(), 2u);
}

TEST(WallDemolition, WallsStayApartWhenTheJoinedOneWouldBeTooLong)
{
	TJunction T;
	WallLimits Limits;
	Limits.MaxLength = 900.0;

	EXPECT_TRUE(DemolishWall(T.Network, T.Stem, Limits));

	EXPECT_EQ(T.Network.GetWallCount(), 2u);
	EXPECT_EQ(T.Network.GetNodeWalls(T.M).size(), 2u);
}

TEST(WallDemolition, UnknownWallChangesNothing)
{
	TJunction T;

	EXPECT_FALSE(DemolishWall(T.Network, WallId{}, WallLimits{}));
	EXPECT_EQ(T.Network.GetWallCount(), 3u);
}
