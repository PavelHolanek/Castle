#include "CastleWallDrawing.h"

#include "CastleCoreConversion.h"
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
		const auto AddCurve = [&](const WallCurve& Curve, const FLinearColor& Color)
		{
			const std::vector<Vec2> Points = Curve.Tessellate(ArcStepRadians);
			for (size_t Index = 1; Index < Points.size(); ++Index)
			{
				Batch.Emplace(ToWorld(Points[Index - 1]), ToWorld(Points[Index]), Color, 0.f, Options.LineThickness, DepthPriority);
			}
		};

		for (const WallId Id : Network.GetWallIds())
		{
			const std::optional<RawWallOutline> Outline = BuildRawOutline(Network, Id);
			if (!Outline)
			{
				continue;
			}

			const std::vector<ELimitViolation> Violations = CheckWall(Network, Id, Options.Limits);
			if (Report && !Violations.empty())
			{
				Report->LimitViolations.Add(DescribeViolations(TEXT("Wall"), WallLabel(Id), Violations));
			}

			AddCurve(Outline->Reference, Violations.empty() ? FLinearColor::Black : ViolationColor);
			if (Outline->Left)
			{
				AddCurve(*Outline->Left, FLinearColor::Red);
			}
			if (Outline->Right)
			{
				AddCurve(*Outline->Right, FLinearColor::Red);
			}
			AddCurve(Outline->StartCap, FLinearColor::Blue);
			AddCurve(Outline->EndCap, FLinearColor::Blue);
		}

		for (const NodeId Id : Network.GetNodeIds())
		{
			const int32 NodeLabel = static_cast<int32>(Id.Value);
			const std::vector<ELimitViolation> Violations = CheckNode(Network, Id, Options.Limits);
			if (Report && !Violations.empty())
			{
				Report->LimitViolations.Add(DescribeViolations(TEXT("Node"), NodeLabel, Violations));
			}
			Lines.DrawPoint(ToWorld(Network.FindNode(Id)->Position), Violations.empty() ? FLinearColor::Green : ViolationColor,
				Options.NodeSize, DepthPriority);

			// A junction outside the limits, such as a too sharp angle, can reach far out. The magenta node says enough.
			if (!Violations.empty())
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
					Lines.DrawPoint(ToWorld(Gap.Path[PointIndex]), CornerColor, Options.NodeSize, DepthPriority);
					if (PointIndex > 0)
					{
						AddCurve(WallCurve::MakeLine(Gap.Path[PointIndex - 1], Gap.Path[PointIndex]), CornerColor);
					}

					// Reported relative to Transform, like the debug actor's node positions.
					const FVector Point = CastleCoreConversion::ToUnreal(Gap.Path[PointIndex], 0.0);
					Points.Add(FString::Printf(TEXT("(%.1f, %.1f)"), Point.X, Point.Y));
				}

				if (!Report)
				{
					continue;
				}
				const WallId First = NodeJunction.Walls[GapIndex].Id;
				const WallId Second = NodeJunction.Walls[(GapIndex + 1) % NodeJunction.Walls.size()].Id;
				FString Description = FString::Printf(TEXT("Node %d, walls %d -> %d, %.1f deg, %s: %s"),
					NodeLabel, WallLabel(First), WallLabel(Second), FMath::RadiansToDegrees(Gap.Angle), GetShapeName(Gap.Shape),
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
}
