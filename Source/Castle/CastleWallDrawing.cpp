#include "CastleWallDrawing.h"

#include "CastleCoreConversion.h"
#include "CastleCore/FloorPlan.h"
#include "CastleCore/WallOutline.h"
#include "Components/LineBatchComponent.h"

namespace
{
	const FLinearColor ViolationColor(1.f, 0.f, 1.f);
	const FLinearColor CornerColor(1.f, 0.5f, 0.f);

	const TCHAR* GetShapeName(Castle::Core::EGapShape Shape)
	{
		using Castle::Core::EGapShape;
		switch (Shape)
		{
		case EGapShape::Unresolved: return TEXT("unresolved");
		case EGapShape::Corner: return TEXT("corner");
		case EGapShape::StraightExtension: return TEXT("straight extension");
		case EGapShape::BeveledExtension: return TEXT("beveled extension");
		case EGapShape::ExtensionToSideMiddle: return TEXT("extension to side middle");
		case EGapShape::SharpOuterCorner: return TEXT("sharp outer corner");
		case EGapShape::BeveledOuterCorner: return TEXT("beveled outer corner");
		}
		return TEXT("unknown");
	}

	FString DescribeViolations(const TCHAR* Kind, int32 Label, const std::vector<Castle::Core::ELimitViolation>& Violations)
	{
		TArray<FString> Names;
		for (const Castle::Core::ELimitViolation Violation : Violations)
		{
			Names.Add(UTF8_TO_TCHAR(Castle::Core::ToString(Violation)));
		}
		return FString::Printf(TEXT("%s %d: %s"), Kind, Label, *FString::Join(Names, TEXT(", ")));
	}

	/** Relative to the drawing's transform, like the debug actor's node positions. */
	FString DescribePoint(Castle::Core::Vec2 Point)
	{
		const FVector Local = CastleCoreConversion::ToUnreal(Point, 0.0);
		return FString::Printf(TEXT("(%.1f, %.1f)"), Local.X, Local.Y);
	}
}

namespace CastleWallDrawing
{
	void DrawNetwork(ULineBatchComponent& Lines, const Castle::Core::WallNetwork& Network, const FTransform& Transform,
		const FCastleWallDrawOptions& Options, FCastleWallDrawReport* Report)
	{
		using namespace Castle::Core;

		Lines.Flush();

		const uint8 DepthPriority = SDPG_Foreground;
		const double ArcStepRadians = FMath::DegreesToRadians(Options.ArcStep);
		const auto ToWorld = [&Transform, &Options](Vec2 Point)
		{
			return Transform.TransformPosition(CastleCoreConversion::ToUnreal(Point, Options.DrawHeight));
		};
		const auto WallLabel = [&Options](WallId Id)
		{
			return Options.WallLabel ? Options.WallLabel(Id) : static_cast<int32>(Id.Value);
		};

		TArray<FBatchedLine> Batch;
		const auto AddLine = [&](Vec2 Start, Vec2 End, const FLinearColor& Color)
		{
			Batch.Emplace(ToWorld(Start), ToWorld(End), Color, 0.f, Options.LineThickness, DepthPriority);
		};
		const auto AddCurve = [&](const WallCurve& Curve, const FLinearColor& Color)
		{
			const std::vector<Vec2> Points = Curve.Tessellate(ArcStepRadians);
			for (size_t Index = 1; Index < Points.size(); ++Index)
			{
				AddLine(Points[Index - 1], Points[Index], Color);
			}
		};

		// The walls themselves are a mesh then; only what is wrong with them needs lines.
		const bool bWalls3D = Options.ViewMode == ECastleWallViewMode::Walls3D;

		// A joint outside the limits, such as a too sharp angle, can reach far out. The magenta node says enough.
		std::vector<NodeId> NodesOutsideLimits;
		for (const NodeId Id : Network.GetNodeIds())
		{
			const std::vector<ELimitViolation> Violations = CheckNode(Network, Id, Options.Limits);
			if (!Violations.empty())
			{
				NodesOutsideLimits.push_back(Id);
				if (Report)
				{
					Report->LimitViolations.Add(DescribeViolations(TEXT("Node"), static_cast<int32>(Id.Value), Violations));
				}
			}
			if (!bWalls3D || !Violations.empty())
			{
				Lines.DrawPoint(ToWorld(Network.FindNode(Id)->Position), Violations.empty() ? FLinearColor::Green : ViolationColor,
					Options.NodeSize, DepthPriority);
			}
		}

		TSet<uint32> WallsOutsideLimits;
		for (const WallId Id : Network.GetWallIds())
		{
			const std::vector<ELimitViolation> Violations = CheckWall(Network, Id, Options.Limits);
			if (!Violations.empty())
			{
				WallsOutsideLimits.Add(Id.Value);
				if (Report)
				{
					Report->LimitViolations.Add(DescribeViolations(TEXT("Wall"), WallLabel(Id), Violations));
				}
			}
		}

		if (Options.ViewMode == ECastleWallViewMode::RawOutlines)
		{
			for (const WallId Id : Network.GetWallIds())
			{
				const RawWallOutline Outline = *BuildRawOutline(Network, Id);
				AddCurve(Outline.Reference, WallsOutsideLimits.Contains(Id.Value) ? ViolationColor : FLinearColor::Black);
				if (Outline.Left)
				{
					AddCurve(*Outline.Left, FLinearColor::Red);
				}
				if (Outline.Right)
				{
					AddCurve(*Outline.Right, FLinearColor::Red);
				}
				AddCurve(Outline.StartCap, FLinearColor::Blue);
				AddCurve(Outline.EndCap, FLinearColor::Blue);
			}
		}
		else
		{
			const FloorPlan Plan = BuildFloorPlan(Network, FloorPlanSettings{.Junctions = Options.Junctions, .SkippedNodes = NodesOutsideLimits});
			for (const WallBody& Body : Plan.Walls)
			{
				const Wall& Data = *Network.FindWall(Body.Wall);
				const bool bFaulty = Body.bConsumedByJoints || WallsOutsideLimits.Contains(Body.Wall.Value);
				if (!bWalls3D || bFaulty)
				{
					AddCurve(*Network.GetReferenceCurve(Body.Wall), bFaulty ? ViolationColor : FLinearColor::Black);
				}
				if (!bWalls3D && Body.Left && Body.Right)
				{
					AddCurve(*Body.Left, FLinearColor::Red);
					AddCurve(*Body.Right, FLinearColor::Red);
					AddLine(Body.Right->GetStart(), Body.Left->GetStart(), FLinearColor::Blue);
					AddLine(Body.Right->GetEnd(), Body.Left->GetEnd(), FLinearColor::Blue);
				}

				if (Report)
				{
					FString Description = FString::Printf(TEXT("Wall %d: cut %.1f at node %u, %.1f at node %u"),
						WallLabel(Body.Wall), Body.StartCut, Data.NodeA.Value, Body.EndCut, Data.NodeB.Value);
					if (Body.bConsumedByJoints)
					{
						Description += TEXT(", consumed by its joints");
					}
					Report->FloorPlanPieces.Add(MoveTemp(Description));
				}
			}

			for (const NodePiece& Piece : Plan.Nodes)
			{
				TArray<FString> Points;
				for (size_t Index = 0; Index < Piece.Polygon.size(); ++Index)
				{
					if (!bWalls3D)
					{
						AddLine(Piece.Polygon[Index], Piece.Polygon[(Index + 1) % Piece.Polygon.size()], CornerColor);
					}
					Points.Add(DescribePoint(Piece.Polygon[Index]));
				}
				if (Report)
				{
					Report->FloorPlanPieces.Add(FString::Printf(TEXT("Node %u piece: %s"), Piece.Node.Value, *FString::Join(Points, TEXT(", "))));
				}
			}

			if (Report)
			{
				for (const NodeId Id : Plan.UnresolvedNodes)
				{
					Report->FloorPlanPieces.Add(FString::Printf(TEXT("Node %u: unresolved"), Id.Value));
				}
			}
		}

		for (const NodeId Id : Network.GetNodeIds())
		{
			if (std::find(NodesOutsideLimits.begin(), NodesOutsideLimits.end(), Id) != NodesOutsideLimits.end())
			{
				continue;
			}

			const Junction NodeJunction = *BuildJunction(Network, Id, Options.Junctions);
			for (size_t GapIndex = 0; GapIndex < NodeJunction.Gaps.size(); ++GapIndex)
			{
				const JunctionGap& Gap = NodeJunction.Gaps[GapIndex];
				if (Gap.Path.empty())
				{
					continue;
				}

				TArray<FString> Points;
				for (size_t PointIndex = 0; PointIndex < Gap.Path.size(); ++PointIndex)
				{
					if (Options.ViewMode == ECastleWallViewMode::RawOutlines)
					{
						Lines.DrawPoint(ToWorld(Gap.Path[PointIndex]), CornerColor, Options.NodeSize, DepthPriority);
						if (PointIndex > 0)
						{
							AddLine(Gap.Path[PointIndex - 1], Gap.Path[PointIndex], CornerColor);
						}
					}
					Points.Add(DescribePoint(Gap.Path[PointIndex]));
				}

				if (!Report)
				{
					continue;
				}
				const WallId First = NodeJunction.Walls[GapIndex].Id;
				const WallId Second = NodeJunction.Walls[(GapIndex + 1) % NodeJunction.Walls.size()].Id;
				FString Description = FString::Printf(TEXT("Node %u, walls %d -> %d, %.1f deg, %s: %s"),
					Id.Value, WallLabel(First), WallLabel(Second), FMath::RadiansToDegrees(Gap.Angle), GetShapeName(Gap.Shape),
					*FString::Join(Points, TEXT(" -> ")));
				if (Gap.Path.size() == 2)
				{
					Description += FString::Printf(TEXT(", length %.1f"), Distance(Gap.Path[0], Gap.Path[1]));
				}
				Report->JunctionCorners.Add(MoveTemp(Description));
			}
		}

		Lines.DrawLines(Batch);
	}

	FCastleWallFaults FindFaults(const Castle::Core::WallNetwork& Network, const Castle::Core::WallLimits& Limits)
	{
		using namespace Castle::Core;

		FCastleWallFaults Faults;
		for (const NodeId Id : Network.GetNodeIds())
		{
			if (!CheckNode(Network, Id, Limits).empty())
			{
				Faults.Nodes.Add(Id.Value);
			}
		}
		for (const WallId Id : Network.GetWallIds())
		{
			if (!CheckWall(Network, Id, Limits).empty())
			{
				Faults.Walls.Add(Id.Value);
			}
		}
		return Faults;
	}
}
