#include "CastleCore/WallNetwork.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>

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

TEST(WallNetwork, FindNearestNodeWithinDistance)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({100.0, 0.0});
	const NodeId Removed = Network.AddNode({60.0, 0.0});
	Network.RemoveNode(Removed);

	EXPECT_EQ(Network.FindNearestNode({40.0, 0.0}, 50.0), A);
	EXPECT_EQ(Network.FindNearestNode({70.0, 10.0}, 50.0), B);
	EXPECT_EQ(Network.FindNearestNode({50.0, 0.0}, 50.0), A);
	EXPECT_FALSE(Network.FindNearestNode({50.0, 30.0}, 50.0).IsValid());
	EXPECT_FALSE(WallNetwork{}.FindNearestNode({0.0, 0.0}, 1000.0).IsValid());
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

TEST(WallNetwork, WidestWallAtANode)
{
	WallNetwork Network;
	const NodeId Center = Network.AddNode({0.0, 0.0});
	const NodeId Alone = Network.AddNode({0.0, 5000.0});
	Network.AddWall(Center, Network.AddNode({1000.0, 0.0}), 50.0, 50.0);
	Network.AddWall(Network.AddNode({0.0, 1000.0}), Center, 200.0, 0.0);

	EXPECT_EQ(Network.GetWidestWall(Center), 200.0);
	EXPECT_EQ(Network.GetWidestWall(Alone), 0.0);
	EXPECT_EQ(Network.GetWidestWall(NodeId{}), 0.0);
}

TEST(WallNetwork, SplitWallKeepsItsIdForTheFirstPart)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({1000.0, 0.0});
	const WallId AB = Network.AddWall(A, B, 30.0, 70.0);

	const NodeId Middle = Network.SplitWall(AB, 0.25);

	ASSERT_TRUE(Middle.IsValid());
	EXPECT_TRUE(IsNearlyEqual(Network.FindNode(Middle)->Position, {250.0, 0.0}));
	EXPECT_EQ(Network.GetNodeCount(), 3u);
	EXPECT_EQ(Network.GetWallCount(), 2u);

	const std::span<const WallId> AtMiddle = Network.GetNodeWalls(Middle);
	ASSERT_EQ(AtMiddle.size(), 2u);
	EXPECT_EQ(AtMiddle[0], AB);
	const WallId Rest = AtMiddle[1];

	const Wall* First = Network.FindWall(AB);
	const Wall* Second = Network.FindWall(Rest);
	ASSERT_NE(Second, nullptr);
	EXPECT_EQ(First->NodeA, A);
	EXPECT_EQ(First->NodeB, Middle);
	EXPECT_EQ(Second->NodeA, Middle);
	EXPECT_EQ(Second->NodeB, B);
	for (const Wall* Part : {First, Second})
	{
		EXPECT_EQ(Part->WidthLeft, 30.0);
		EXPECT_EQ(Part->WidthRight, 70.0);
		EXPECT_EQ(Part->Bulge, 0.0);
	}

	// The old end holds the second part instead of the wall.
	EXPECT_EQ(Network.GetNodeWalls(A).size(), 1u);
	ASSERT_EQ(Network.GetNodeWalls(B).size(), 1u);
	EXPECT_EQ(Network.GetNodeWalls(B)[0], Rest);
}

TEST(WallNetwork, SplitArcGivesArcsOfTheSameCircle)
{
	WallNetwork Network;
	const WallId Arc = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({200.0, 0.0}), 20.0, 20.0, 1.0);
	const WallCurve Original = *Network.GetReferenceCurve(Arc);

	const NodeId Middle = Network.SplitWall(Arc, 0.25);

	ASSERT_TRUE(Middle.IsValid());
	EXPECT_TRUE(IsNearlyEqual(Network.FindNode(Middle)->Position, Original.PointAt(0.25)));

	const WallCurve First = *Network.GetReferenceCurve(Arc);
	const WallCurve Second = *Network.GetReferenceCurve(Network.GetNodeWalls(Middle)[1]);
	for (const WallCurve& Part : {First, Second})
	{
		ASSERT_TRUE(Part.IsArc());
		EXPECT_TRUE(IsNearlyEqual(Part.GetCenter(), Original.GetCenter()));
		EXPECT_NEAR(Part.GetRadius(), Original.GetRadius(), 1e-9);
	}
	EXPECT_NEAR(First.GetSweepAngle(), 0.25 * Pi, 1e-9);
	EXPECT_NEAR(Second.GetSweepAngle(), 0.75 * Pi, 1e-9);
	EXPECT_TRUE(IsNearlyEqual(Second.PointAt(0.5), Original.PointAt(0.625)));
}

TEST(WallNetwork, SplitNeedsAPointBetweenTheEnds)
{
	WallNetwork Network;
	const WallId AB = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 50.0, 50.0);

	for (const double T : {0.0, 1.0, -0.5, 1.5, std::numeric_limits<double>::quiet_NaN()})
	{
		EXPECT_FALSE(Network.SplitWall(AB, T).IsValid());
	}
	EXPECT_FALSE(Network.SplitWall(WallId{}, 0.5).IsValid());
	EXPECT_EQ(Network.GetNodeCount(), 2u);
	EXPECT_EQ(Network.GetWallCount(), 1u);
}

TEST(WallNetwork, FindWallAtPointOverItsBody)
{
	// 1000 long along X, 100 wide to the left and 20 to the right.
	WallNetwork Network;
	const WallId AB = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 100.0, 20.0);

	const std::optional<WallPoint> Hit = Network.FindWallAt({300.0, 90.0});
	ASSERT_TRUE(Hit);
	EXPECT_EQ(Hit->Wall, AB);
	EXPECT_NEAR(Hit->T, 0.3, 1e-9);
	EXPECT_TRUE(IsNearlyEqual(Hit->Position, {300.0, 0.0}));

	EXPECT_TRUE(Network.FindWallAt({300.0, -20.0}));
	EXPECT_FALSE(Network.FindWallAt({300.0, -30.0}));
	EXPECT_FALSE(Network.FindWallAt({300.0, 110.0}));

	// The reach helps only where the wall is thinner.
	EXPECT_TRUE(Network.FindWallAt({300.0, -30.0}, 40.0));
	EXPECT_FALSE(Network.FindWallAt({300.0, 110.0}, 40.0));

	// Past the ends the wall doesn't count, even within its width.
	EXPECT_FALSE(Network.FindWallAt({-5.0, 10.0}));
	EXPECT_FALSE(Network.FindWallAt({1005.0, 10.0}));
}

TEST(WallNetwork, FindWallAtPrefersTheNearestReferenceLine)
{
	// Two parallel walls 80 apart, overlapping by 20.
	WallNetwork Network;
	const WallId Lower = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 50.0, 50.0);
	const WallId Upper = Network.AddWall(Network.AddNode({0.0, 80.0}), Network.AddNode({1000.0, 80.0}), 50.0, 50.0);

	EXPECT_EQ(Network.FindWallAt({500.0, 35.0})->Wall, Lower);
	EXPECT_EQ(Network.FindWallAt({500.0, 45.0})->Wall, Upper);

	Network.RemoveWall(Lower);
	EXPECT_EQ(Network.FindWallAt({500.0, 35.0})->Wall, Upper);
}

TEST(WallNetwork, FindWallAtOnAnArc)
{
	// A half circle around (500, 0) with radius 500, bowing to -Y.
	WallNetwork Network;
	const WallId Arc = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 50.0, 50.0, 1.0);

	const std::optional<WallPoint> Hit = Network.FindWallAt({500.0, -540.0});
	ASSERT_TRUE(Hit);
	EXPECT_EQ(Hit->Wall, Arc);
	EXPECT_NEAR(Hit->T, 0.5, 1e-9);
	EXPECT_TRUE(IsNearlyEqual(Hit->Position, {500.0, -500.0}));

	EXPECT_FALSE(Network.FindWallAt({500.0, -560.0}));
	EXPECT_FALSE(Network.FindWallAt({500.0, 0.0}));
}

TEST(WallNetwork, MergeUndoesASplit)
{
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({1000.0, 0.0});
	const WallId AB = Network.AddWall(A, B, 30.0, 70.0);
	const NodeId Middle = Network.SplitWall(AB, 0.4);

	EXPECT_EQ(Network.MergeWalls(Middle), AB);

	EXPECT_EQ(Network.FindNode(Middle), nullptr);
	EXPECT_EQ(Network.GetNodeCount(), 2u);
	EXPECT_EQ(Network.GetWallCount(), 1u);
	const Wall* Merged = Network.FindWall(AB);
	EXPECT_EQ(Merged->NodeA, A);
	EXPECT_EQ(Merged->NodeB, B);
	EXPECT_EQ(Merged->WidthLeft, 30.0);
	EXPECT_EQ(Merged->WidthRight, 70.0);
	EXPECT_EQ(Merged->Bulge, 0.0);
	ASSERT_EQ(Network.GetNodeWalls(B).size(), 1u);
	EXPECT_EQ(Network.GetNodeWalls(B)[0], AB);
}

TEST(WallNetwork, MergeUndoesASplitOfAnArc)
{
	WallNetwork Network;
	const WallId Arc = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 50.0, 50.0, -0.6);

	EXPECT_EQ(Network.MergeWalls(Network.SplitWall(Arc, 0.3)), Arc);
	EXPECT_NEAR(Network.FindWall(Arc)->Bulge, -0.6, 1e-12);
}

TEST(WallNetwork, MergeKeepsTheDirectionOfTheOlderWall)
{
	// Both walls run away from B towards A, and the older one starts at the node, so the joined wall runs from C to A.
	// Seen that way, the widths of both are 30 to the left and 70 to the right.
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId Middle = Network.AddNode({400.0, 0.0});
	const NodeId C = Network.AddNode({1000.0, 0.0});
	const WallId Older = Network.AddWall(Middle, A, 30.0, 70.0);
	Network.AddWall(C, Middle, 30.0, 70.0);

	ASSERT_EQ(Network.MergeWalls(Middle), Older);

	const Wall* Merged = Network.FindWall(Older);
	EXPECT_EQ(Merged->NodeA, C);
	EXPECT_EQ(Merged->NodeB, A);
	EXPECT_EQ(Merged->WidthLeft, 30.0);
	EXPECT_EQ(Merged->WidthRight, 70.0);
}

TEST(WallNetwork, MergeOnlyWallsThatContinueEachOther)
{
	const auto CanMerge = [](Vec2 End, double EndWidthLeft, double EndBulge)
	{
		WallNetwork Network;
		const NodeId Middle = Network.AddNode({500.0, 0.0});
		Network.AddWall(Network.AddNode({0.0, 0.0}), Middle, 50.0, 50.0);
		Network.AddWall(Middle, Network.AddNode(End), EndWidthLeft, 50.0, EndBulge);
		return Network.MergeWalls(Middle).IsValid();
	};

	EXPECT_TRUE(CanMerge({1000.0, 0.0}, 50.0, 0.0));
	EXPECT_FALSE(CanMerge({500.0, 500.0}, 50.0, 0.0));
	EXPECT_FALSE(CanMerge({1000.0, 1.0}, 50.0, 0.0));
	EXPECT_FALSE(CanMerge({0.0, 0.0}, 50.0, 0.0));
	EXPECT_FALSE(CanMerge({1000.0, 0.0}, 60.0, 0.0));
	EXPECT_FALSE(CanMerge({1000.0, 0.0}, 50.0, 0.3));
}

TEST(WallNetwork, MergeNeedsExactlyTwoWallsWithDifferentFarEnds)
{
	// A round tower: two half circles between A and B.
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({400.0, 0.0});
	Network.AddWall(A, B, 50.0, 50.0, 1.0);
	Network.AddWall(B, A, 50.0, 50.0, 1.0);
	EXPECT_FALSE(Network.MergeWalls(B).IsValid());

	// Three walls in a row of which two would continue each other.
	const NodeId C = Network.AddNode({1000.0, 0.0});
	Network.AddWall(B, C, 50.0, 50.0);
	EXPECT_FALSE(Network.MergeWalls(B).IsValid());

	EXPECT_FALSE(Network.MergeWalls(C).IsValid());
	EXPECT_FALSE(Network.MergeWalls(NodeId{}).IsValid());
	EXPECT_EQ(Network.GetWallCount(), 3u);
}
