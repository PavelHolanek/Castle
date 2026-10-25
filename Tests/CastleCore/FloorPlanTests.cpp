#include "CastleCore/FloorPlan.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>

using namespace Castle::Core;

namespace
{
	constexpr double Tolerance = 1e-6;

	/** Allowed distance from a boundary in the geometric checks; points closer count as on it. */
	constexpr double BoundaryTolerance = 1e-4;

	void ExpectNear(Vec2 Actual, Vec2 Expected, double Within = Tolerance)
	{
		EXPECT_NEAR(Actual.X, Expected.X, Within);
		EXPECT_NEAR(Actual.Y, Expected.Y, Within);
	}

	Vec2 Direction(double Degrees)
	{
		return PointOnCircle({0.0, 0.0}, 1.0, DegreesToRadians(Degrees));
	}

	/** Straight walls around a node at the origin. */
	struct Star
	{
		WallNetwork Network;
		NodeId Center = Network.AddNode({0.0, 0.0});

		WallId AddOutgoing(double Degrees, double WidthLeft, double WidthRight, double Length = 1000.0)
		{
			return Network.AddWall(Center, Network.AddNode(Direction(Degrees) * Length), WidthLeft, WidthRight);
		}

		WallId AddIncoming(double Degrees, double WidthLeft, double WidthRight, double Length = 1000.0)
		{
			return Network.AddWall(Network.AddNode(Direction(Degrees) * Length), Center, WidthLeft, WidthRight);
		}
	};

	const WallBody& FindBody(const FloorPlan& Plan, WallId Id)
	{
		const auto Found = std::find_if(Plan.Walls.begin(), Plan.Walls.end(), [Id](const WallBody& Body) { return Body.Wall == Id; });
		EXPECT_NE(Found, Plan.Walls.end());
		return *Found;
	}

	const NodePiece* FindPiece(const FloorPlan& Plan, NodeId Id)
	{
		const auto Found = std::find_if(Plan.Nodes.begin(), Plan.Nodes.end(), [Id](const NodePiece& Piece) { return Piece.Node == Id; });
		return Found == Plan.Nodes.end() ? nullptr : &*Found;
	}

	bool HasVertex(const std::vector<Vec2>& Polygon, Vec2 Point)
	{
		return std::any_of(Polygon.begin(), Polygon.end(), [Point](Vec2 Vertex) { return IsNearlyEqual(Vertex, Point, 1e-3); });
	}

	double DistanceToSegment(Vec2 Point, Vec2 Start, Vec2 End)
	{
		const Vec2 Along = End - Start;
		const double LengthSquared = Dot(Along, Along);
		const double T = LengthSquared > 0.0 ? std::clamp(Dot(Point - Start, Along) / LengthSquared, 0.0, 1.0) : 0.0;
		return Distance(Point, Start + Along * T);
	}

	bool IsOnBoundary(const std::vector<Vec2>& Polygon, Vec2 Point)
	{
		for (size_t Index = 0; Index < Polygon.size(); ++Index)
		{
			if (DistanceToSegment(Point, Polygon[Index], Polygon[(Index + 1) % Polygon.size()]) <= BoundaryTolerance)
			{
				return true;
			}
		}
		return false;
	}

	/** Inside and not on the boundary. */
	bool IsStrictlyInside(const std::vector<Vec2>& Polygon, Vec2 Point)
	{
		if (IsOnBoundary(Polygon, Point))
		{
			return false;
		}
		bool bInside = false;
		for (size_t Index = 0, Previous = Polygon.size() - 1; Index < Polygon.size(); Previous = Index++)
		{
			const Vec2 A = Polygon[Index];
			const Vec2 B = Polygon[Previous];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	/** The segments cross at a single point inside both; touching or overlapping along a line doesn't count. */
	bool CrossProperly(Vec2 A1, Vec2 A2, Vec2 B1, Vec2 B2)
	{
		const auto Side = [](Vec2 From, Vec2 To, Vec2 Point)
		{
			const double Value = Cross(Normalized(To - From), Point - From);
			return Value > BoundaryTolerance ? 1 : Value < -BoundaryTolerance ? -1 : 0;
		};
		return Side(B1, B2, A1) * Side(B1, B2, A2) < 0 && Side(A1, A2, B1) * Side(A1, A2, B2) < 0;
	}

	bool IsSimple(const std::vector<Vec2>& Polygon)
	{
		const size_t Count = Polygon.size();
		for (size_t First = 0; First < Count; ++First)
		{
			for (size_t Second = First + 2; Second < Count; ++Second)
			{
				if (First == 0 && Second == Count - 1)
				{
					continue;
				}
				if (CrossProperly(Polygon[First], Polygon[(First + 1) % Count], Polygon[Second], Polygon[(Second + 1) % Count]))
				{
					return false;
				}
			}
		}
		return true;
	}

	bool Overlap(const std::vector<Vec2>& A, const std::vector<Vec2>& B)
	{
		for (size_t First = 0; First < A.size(); ++First)
		{
			const Vec2 A1 = A[First];
			const Vec2 A2 = A[(First + 1) % A.size()];
			if (IsStrictlyInside(B, A1) || IsStrictlyInside(B, (A1 + A2) * 0.5))
			{
				return true;
			}
			for (size_t Second = 0; Second < B.size(); ++Second)
			{
				if (CrossProperly(A1, A2, B[Second], B[(Second + 1) % B.size()]))
				{
					return true;
				}
			}
		}
		for (size_t Second = 0; Second < B.size(); ++Second)
		{
			if (IsStrictlyInside(A, B[Second]) || IsStrictlyInside(A, (B[Second] + B[(Second + 1) % B.size()]) * 0.5))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Checks what must hold at any resolved node: the piece is a simple counterclockwise polygon, every wall's cut lies on
	 * its outline (no gaps), and neither the piece nor the wall bodies overlap each other.
	 */
	void ExpectSoundJoint(const WallNetwork& Network, const FloorPlan& Plan, NodeId Node)
	{
		const NodePiece* Piece = FindPiece(Plan, Node);
		std::vector<std::vector<Vec2>> Bodies;
		for (const WallId Attached : Network.GetNodeWalls(Node))
		{
			const WallBody& Body = FindBody(Plan, Attached);
			ASSERT_FALSE(Body.bConsumedByJoints);
			ASSERT_TRUE(Body.Left && Body.Right);
			Bodies.push_back(GetBodyPolygon(Body, DegreesToRadians(5.0)));
			EXPECT_GT(GetSignedArea(Bodies.back()), 0.0);

			if (Piece)
			{
				const bool bStartsHere = Network.FindWall(Attached)->NodeA == Node;
				EXPECT_TRUE(IsOnBoundary(Piece->Polygon, bStartsHere ? Body.Left->GetStart() : Body.Left->GetEnd()));
				EXPECT_TRUE(IsOnBoundary(Piece->Polygon, bStartsHere ? Body.Right->GetStart() : Body.Right->GetEnd()));
			}
		}

		if (Piece)
		{
			EXPECT_TRUE(IsSimple(Piece->Polygon));
			EXPECT_GT(GetSignedArea(Piece->Polygon), 0.0);
			for (const std::vector<Vec2>& Body : Bodies)
			{
				EXPECT_FALSE(Overlap(Piece->Polygon, Body));
			}
		}
		for (size_t First = 0; First < Bodies.size(); ++First)
		{
			for (size_t Second = First + 1; Second < Bodies.size(); ++Second)
			{
				EXPECT_FALSE(Overlap(Bodies[First], Bodies[Second]));
			}
		}
	}
}

TEST(FloorPlan, FreeEndKeepsPlainCap)
{
	WallNetwork Network;
	const WallId Wall = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 50.0, 30.0);

	const FloorPlan Plan = BuildFloorPlan(Network);
	const WallBody& Body = FindBody(Plan, Wall);
	EXPECT_EQ(Body.StartCut, 0.0);
	EXPECT_EQ(Body.EndCut, 0.0);
	ExpectNear(Body.Left->GetStart(), {0.0, 50.0});
	ExpectNear(Body.Right->GetEnd(), {1000.0, -30.0});
	EXPECT_TRUE(Plan.Nodes.empty());
	EXPECT_TRUE(Plan.UnresolvedNodes.empty());

	const std::vector<Vec2> Polygon = GetBodyPolygon(Body, 0.1);
	ASSERT_EQ(Polygon.size(), 4u);
	EXPECT_NEAR(GetSignedArea(Polygon), 80000.0, Tolerance);
}

TEST(FloorPlan, WallsInLineLeaveNoPiece)
{
	Star Joint;
	const WallId East = Joint.AddOutgoing(0.0, 100.0, 100.0);
	const WallId West = Joint.AddIncoming(180.0, 100.0, 100.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	EXPECT_NEAR(FindBody(Plan, East).StartCut, 0.0, Tolerance);
	EXPECT_NEAR(FindBody(Plan, West).EndCut, 0.0, Tolerance);
	EXPECT_FALSE(FindPiece(Plan, Joint.Center));
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, RightAngleCornerIsASquare)
{
	Star Joint;
	const WallId East = Joint.AddOutgoing(0.0, 100.0, 100.0);
	const WallId North = Joint.AddOutgoing(90.0, 100.0, 100.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	EXPECT_NEAR(FindBody(Plan, East).StartCut, 100.0, Tolerance);
	EXPECT_NEAR(FindBody(Plan, North).StartCut, 100.0, Tolerance);

	const NodePiece* Piece = FindPiece(Plan, Joint.Center);
	ASSERT_TRUE(Piece);
	ASSERT_EQ(Piece->Polygon.size(), 4u);
	for (const Vec2 Corner : {Vec2{100.0, -100.0}, Vec2{100.0, 100.0}, Vec2{-100.0, 100.0}, Vec2{-100.0, -100.0}})
	{
		EXPECT_TRUE(HasVertex(Piece->Polygon, Corner));
	}
	EXPECT_NEAR(GetSignedArea(Piece->Polygon), 40000.0, Tolerance);
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, BeveledRightAngleLosesTheOuterCorner)
{
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(90.0, 100.0, 100.0);
	Joint.Network.SetCornerStyle(Joint.Center, ECornerStyle::Beveled);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	const NodePiece* Piece = FindPiece(Plan, Joint.Center);
	ASSERT_TRUE(Piece);
	EXPECT_EQ(Piece->Polygon.size(), 5u);
	EXPECT_FALSE(HasVertex(Piece->Polygon, {-100.0, -100.0}));
	EXPECT_NEAR(GetSignedArea(Piece->Polygon), 35000.0, Tolerance);
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, TJunctionPieceSpansTheThroughWall)
{
	Star Joint;
	const WallId East = Joint.AddOutgoing(0.0, 150.0, 150.0);
	const WallId West = Joint.AddIncoming(180.0, 150.0, 150.0);
	const WallId North = Joint.AddOutgoing(90.0, 60.0, 60.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	EXPECT_NEAR(FindBody(Plan, North).StartCut, 150.0, Tolerance);
	EXPECT_NEAR(FindBody(Plan, East).StartCut, 60.0, Tolerance);
	EXPECT_NEAR(FindBody(Plan, West).EndCut, 60.0, Tolerance);

	const NodePiece* Piece = FindPiece(Plan, Joint.Center);
	ASSERT_TRUE(Piece);
	ASSERT_EQ(Piece->Polygon.size(), 4u);
	EXPECT_NEAR(GetSignedArea(Piece->Polygon), 120.0 * 300.0, Tolerance);
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, AcuteJointIsCutWhereTheSidesCross)
{
	Star Joint;
	const WallId First = Joint.AddOutgoing(0.0, 100.0, 100.0);
	const WallId Second = Joint.AddOutgoing(60.0, 100.0, 100.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	const double Cut = 100.0 / std::tan(DegreesToRadians(30.0));
	EXPECT_NEAR(FindBody(Plan, First).StartCut, Cut, Tolerance);
	EXPECT_NEAR(FindBody(Plan, Second).StartCut, Cut, Tolerance);

	// The inner crossing, the cut across the second wall, and the sharp outer corner.
	const NodePiece* Piece = FindPiece(Plan, Joint.Center);
	ASSERT_TRUE(Piece);
	ASSERT_EQ(Piece->Polygon.size(), 4u);
	EXPECT_TRUE(HasVertex(Piece->Polygon, Direction(30.0) * 200.0));
	EXPECT_TRUE(HasVertex(Piece->Polygon, Direction(30.0) * -200.0));
	EXPECT_TRUE(HasVertex(Piece->Polygon, {0.0, 200.0}));
	EXPECT_TRUE(HasVertex(Piece->Polygon, {Cut, -100.0}));
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, ThickerWallNoLongerSticksOutAtSharpAngle)
{
	// Outer angle over 270 degrees: the raw cap of the thicker wall reaches across the thinner wall's side.
	Star Joint;
	const WallId Thick = Joint.AddOutgoing(0.0, 150.0, 150.0);
	const WallId Thin = Joint.AddOutgoing(50.0, 40.0, 40.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);

	// Both are cut across the crossing of their inner sides, the thick one where the thin side reaches its side.
	const double ThinSideReach = (150.0 + 40.0 * std::cos(DegreesToRadians(50.0))) / std::sin(DegreesToRadians(50.0));
	const Vec2 Crossing = Direction(50.0) * ThinSideReach - PerpLeft(Direction(50.0)) * 40.0;
	EXPECT_NEAR(Crossing.Y, 150.0, Tolerance);
	EXPECT_NEAR(FindBody(Plan, Thick).StartCut, Crossing.X, Tolerance);
	EXPECT_NEAR(FindBody(Plan, Thin).StartCut, ThinSideReach, Tolerance);
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, SideExtensionsBelongToTheNode)
{
	// 135 degrees, 400 against 80: the wide side runs on behind the node, the other side is beveled.
	Star Joint;
	const WallId Wide = Joint.AddOutgoing(0.0, 200.0, 200.0);
	const WallId Thin = Joint.AddOutgoing(135.0, 40.0, 40.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	EXPECT_NEAR(FindBody(Plan, Wide).StartCut, 0.0, Tolerance);

	// The bevel of the 225 degree side meets the thin wall farther out than the straight extension of the other side:
	// it runs from the wide side's end (0, -200) to the thin side at the maximum length 400.
	const Vec2 ThinSideStart = PerpLeft(Direction(135.0)) * 40.0;
	const Vec2 FromWideSide = ThinSideStart - Vec2{0.0, -200.0};
	const double Projection = Dot(FromWideSide, Direction(135.0));
	const double BevelReach = -Projection + std::sqrt(Projection * Projection - Dot(FromWideSide, FromWideSide) + 400.0 * 400.0);
	EXPECT_NEAR(FindBody(Plan, Thin).StartCut, BevelReach, Tolerance);

	const NodePiece* Piece = FindPiece(Plan, Joint.Center);
	ASSERT_TRUE(Piece);
	EXPECT_EQ(Piece->Polygon.size(), 5u);
	EXPECT_TRUE(HasVertex(Piece->Polygon, {-(200.0 - 40.0 * std::sqrt(2.0)), 200.0}));
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, ThreeWallsOfDifferentWidths)
{
	// The case from the debug actor: 125, 105 and 130 degrees, one wall ending at the node.
	Star Joint;
	Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(125.0, 80.0, 80.0);
	Joint.AddIncoming(230.0, 120.0, 120.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	ASSERT_TRUE(FindPiece(Plan, Joint.Center));
	ExpectSoundJoint(Joint.Network, Plan, Joint.Center);
}

TEST(FloorPlan, JointsHoldUpAcrossAnglesAndWidths)
{
	struct Widths
	{
		double First;
		double Second;
	};

	for (const ECornerStyle Style : {ECornerStyle::Sharp, ECornerStyle::Beveled})
	{
		for (const Widths Pair : {Widths{100.0, 100.0}, Widths{150.0, 40.0}, Widths{40.0, 150.0}, Widths{200.0, 70.0}})
		{
			for (const double Degrees : {30.0, 45.0, 60.0, 90.0, 110.0, 135.0, 150.0, 160.0, 170.0, 175.0, 180.0, 185.0,
				190.0, 200.0, 210.0, 225.0, 250.0, 270.0, 290.0, 300.0, 315.0, 330.0})
			{
				Star Joint;
				Joint.AddOutgoing(0.0, Pair.First, Pair.First);
				Joint.AddOutgoing(Degrees, Pair.Second, Pair.Second);
				Joint.Network.SetCornerStyle(Joint.Center, Style);
				SCOPED_TRACE(std::to_string(Degrees) + " deg, widths " + std::to_string(Pair.First) + " / "
					+ std::to_string(Pair.Second) + (Style == ECornerStyle::Sharp ? ", sharp" : ", beveled"));

				ExpectSoundJoint(Joint.Network, BuildFloorPlan(Joint.Network), Joint.Center);
			}
		}
	}
}

TEST(FloorPlan, JointsMayConsumeAShortWall)
{
	// A 150 long wall with 60 degree joints at both ends, each needing 50 / tan(30) = 86.6 of it.
	WallNetwork Network;
	const NodeId A = Network.AddNode({0.0, 0.0});
	const NodeId B = Network.AddNode({150.0, 0.0});
	const WallId Short = Network.AddWall(A, B, 50.0, 50.0);
	Network.AddWall(A, Network.AddNode(Direction(60.0) * 1000.0), 50.0, 50.0);
	Network.AddWall(B, Network.AddNode(Vec2{150.0, 0.0} + Direction(120.0) * 1000.0), 50.0, 50.0);

	const FloorPlan Plan = BuildFloorPlan(Network);
	const WallBody& Body = FindBody(Plan, Short);
	EXPECT_TRUE(Body.bConsumedByJoints);
	EXPECT_FALSE(Body.Left);
	EXPECT_TRUE(GetBodyPolygon(Body, 0.1).empty());
}

TEST(FloorPlan, NodeWithAnArcStaysUnresolved)
{
	Star Joint;
	const WallId Straight = Joint.AddOutgoing(0.0, 100.0, 100.0);
	const WallId Arc = Joint.Network.AddWall(Joint.Center, Joint.Network.AddNode({0.0, 1000.0}), 100.0, 100.0, 0.3);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network);
	ASSERT_EQ(Plan.UnresolvedNodes.size(), 1u);
	EXPECT_EQ(Plan.UnresolvedNodes[0], Joint.Center);
	EXPECT_FALSE(FindPiece(Plan, Joint.Center));
	EXPECT_EQ(FindBody(Plan, Straight).StartCut, 0.0);
	EXPECT_EQ(FindBody(Plan, Arc).StartCut, 0.0);
}

TEST(FloorPlan, SkippedNodeStaysUnresolved)
{
	Star Joint;
	const WallId East = Joint.AddOutgoing(0.0, 100.0, 100.0);
	Joint.AddOutgoing(90.0, 100.0, 100.0);

	const FloorPlan Plan = BuildFloorPlan(Joint.Network, FloorPlanSettings{.SkippedNodes = {Joint.Center}});
	ASSERT_EQ(Plan.UnresolvedNodes.size(), 1u);
	EXPECT_FALSE(FindPiece(Plan, Joint.Center));
	EXPECT_EQ(FindBody(Plan, East).StartCut, 0.0);
}

TEST(FloorPlan, ArcBodyIsARingSector)
{
	// Counterclockwise half circle of radius 500; the left side is the inner one.
	WallNetwork Network;
	const WallId Arc = Network.AddWall(Network.AddNode({0.0, 0.0}), Network.AddNode({1000.0, 0.0}), 100.0, 100.0, 1.0);

	const std::vector<Vec2> Polygon = GetBodyPolygon(FindBody(BuildFloorPlan(Network), Arc), DegreesToRadians(0.5));
	const double Exact = 0.5 * Pi * (600.0 * 600.0 - 400.0 * 400.0);
	EXPECT_NEAR(GetSignedArea(Polygon), Exact, Exact * 1e-4);
}

TEST(FloorPlan, GeometryChecksCatchProblems)
{
	// Raw rectangles of an L corner overlap at the node, a bow tie crosses itself.
	const std::vector<Vec2> East = {{0.0, -100.0}, {1000.0, -100.0}, {1000.0, 100.0}, {0.0, 100.0}};
	const std::vector<Vec2> North = {{-100.0, 0.0}, {100.0, 0.0}, {100.0, 1000.0}, {-100.0, 1000.0}};
	const std::vector<Vec2> Beside = {{0.0, 100.0}, {1000.0, 100.0}, {1000.0, 300.0}, {0.0, 300.0}};
	EXPECT_TRUE(Overlap(East, North));
	EXPECT_FALSE(Overlap(East, Beside));
	EXPECT_FALSE(IsSimple({{0.0, 0.0}, {100.0, 100.0}, {100.0, 0.0}, {0.0, 100.0}}));
	EXPECT_TRUE(IsSimple(East));
	EXPECT_FALSE(IsOnBoundary(East, {500.0, 0.0}));
	EXPECT_TRUE(IsOnBoundary(East, {500.0, 100.0}));
}
