#include "CastleWallMesh.h"

#include "CastleCoreConversion.h"
#include "CastleCore/FloorPlan.h"
#include "CompGeom/PolygonTriangulation.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "VectorUtil.h"

#include <algorithm>

namespace
{
	using UE::Geometry::FDynamicMesh3;
	using UE::Geometry::FDynamicMeshMaterialAttribute;
	using UE::Geometry::FDynamicMeshNormalOverlay;
	using UE::Geometry::FIndex3i;

	enum EMaterialSlot : int32
	{
		WallSlot,
		FaultSlot,
		HighlightSlot,
		SlotCount,
	};

	/** Where new triangles go: the mesh, its normals and the material slot they use. */
	struct FMeshTarget
	{
		FDynamicMesh3& Mesh;
		FDynamicMeshNormalOverlay& Normals;
		FDynamicMeshMaterialAttribute& MaterialIDs;
		int32 Slot = WallSlot;
	};

	/** Appends the triangle wound so that it faces Normal, which also becomes its flat normal. */
	void AppendFacing(const FMeshTarget& Target, int32 A, int32 B, int32 C, const FVector3d& Normal, int32 NormalElement)
	{
		// The engine's notion of facing, which accounts for Unreal's left-handed coordinates.
		const FDynamicMesh3& Mesh = Target.Mesh;
		if (UE::Geometry::VectorUtil::Normal(Mesh.GetVertex(A), Mesh.GetVertex(B), Mesh.GetVertex(C)).Dot(Normal) < 0.0)
		{
			Swap(B, C);
		}

		const int32 Triangle = Target.Mesh.AppendTriangle(A, B, C);
		if (Triangle >= 0)
		{
			Target.Normals.SetTriangle(Triangle, FIndex3i(NormalElement, NormalElement, NormalElement));
			Target.MaterialIDs.SetValue(Triangle, Target.Slot);
		}
	}

	/** Makes Slot of Mesh show BaseMaterial in Color. */
	void SetSlotColor(UDynamicMeshComponent& Mesh, int32 Slot, UMaterialInterface* BaseMaterial, const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Mesh.GetMaterial(Slot));
		if (BaseMaterial && (!Material || Material->Parent != BaseMaterial))
		{
			Material = UMaterialInstanceDynamic::Create(BaseMaterial, &Mesh);
			Mesh.SetMaterial(Slot, Material);
		}
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}

	/** Extrudes a counterclockwise polygon of the core plane from Z = 0 to Height. */
	void AppendPrism(const FMeshTarget& Target, const std::vector<Castle::Core::Vec2>& Polygon, double Height)
	{
		FDynamicMesh3& Mesh = Target.Mesh;
		FDynamicMeshNormalOverlay& Normals = Target.Normals;

		using namespace Castle::Core;

		if (Polygon.size() < 3)
		{
			return;
		}

		TArray<FVector2d> Outline;
		TArray<int32> Bottom;
		TArray<int32> Top;
		for (const Vec2 Point : Polygon)
		{
			const FVector Position = CastleCoreConversion::ToUnreal(Point, 0.0);
			Outline.Add(FVector2d(Position.X, Position.Y));
			Bottom.Add(Mesh.AppendVertex(FVector3d(Position.X, Position.Y, 0.0)));
			Top.Add(Mesh.AppendVertex(FVector3d(Position.X, Position.Y, Height)));
		}

		TArray<FIndex3i> CapTriangles;
		PolygonTriangulation::TriangulateSimplePolygon(Outline, CapTriangles, false);
		const int32 Up = Normals.AppendElement(FVector3f::UnitZ());
		const int32 Down = Normals.AppendElement(-FVector3f::UnitZ());
		for (const FIndex3i& Triangle : CapTriangles)
		{
			AppendFacing(Target, Top[Triangle.A], Top[Triangle.B], Top[Triangle.C], FVector3d::UnitZ(), Up);
			AppendFacing(Target, Bottom[Triangle.A], Bottom[Triangle.B], Bottom[Triangle.C], -FVector3d::UnitZ(), Down);
		}

		// Counterclockwise in the core plane, so each edge faces out to its right there.
		for (int32 Index = 0; Index < Outline.Num(); ++Index)
		{
			const int32 Next = (Index + 1) % Outline.Num();
			const Vec2 Edge = Polygon[Next] - Polygon[Index];
			if (Length(Edge) <= DistanceTolerance)
			{
				continue;
			}

			const FVector3d Outward = CastleCoreConversion::ToUnreal(Normalized(Vec2{Edge.Y, -Edge.X}), 0.0);
			const int32 Side = Normals.AppendElement(FVector3f(Outward));
			AppendFacing(Target, Bottom[Index], Bottom[Next], Top[Next], Outward, Side);
			AppendFacing(Target, Bottom[Index], Top[Next], Top[Index], Outward, Side);
		}
	}
}

namespace CastleWallMesh
{
	void ShowWalls(UDynamicMeshComponent& Mesh, const Castle::Core::WallNetwork* Network, const FCastleWallDrawOptions& Options,
		const FCastleWallStyle& Style, UMaterialInterface* BaseMaterial)
	{
		using namespace Castle::Core;

		FDynamicMesh3 Walls;
		Walls.EnableAttributes();
		Walls.Attributes()->EnableMaterialID();
		if (Network)
		{
			FMeshTarget Target{.Mesh = Walls, .Normals = *Walls.Attributes()->PrimaryNormals(), .MaterialIDs = *Walls.Attributes()->GetMaterialID()};
			const double ArcStepRadians = FMath::DegreesToRadians(Options.ArcStep);

			// Nodes outside the limits are left unresolved, their walls run up to them in full.
			const FCastleWallFaults Faults = CastleWallDrawing::FindFaults(*Network, Options.Limits);
			FloorPlanSettings Settings{.Junctions = Options.Junctions};
			for (const uint32 FaultyNode : Faults.Nodes)
			{
				Settings.SkippedNodes.push_back(NodeId{FaultyNode});
			}

			const FloorPlan Plan = BuildFloorPlan(*Network, Settings);
			for (const WallBody& Body : Plan.Walls)
			{
				Target.Slot = Options.HighlightedWalls.Contains(Body.Wall.Value) ? HighlightSlot
					: Faults.Walls.Contains(Body.Wall.Value) ? FaultSlot
					: WallSlot;
				AppendPrism(Target, GetBodyPolygon(Body, ArcStepRadians), Style.Height);
			}
			Target.Slot = WallSlot;
			for (const NodePiece& Piece : Plan.Nodes)
			{
				AppendPrism(Target, Piece.Polygon, Style.Height);
			}

			// Over them goes the joint those nodes would have, cut down to a circle. Where there is none to cut down,
			// such as walls running on top of each other, the circle alone marks the node.
			if (!Faults.Nodes.IsEmpty())
			{
				const FloorPlan Unclipped = BuildFloorPlan(*Network, FloorPlanSettings{.Junctions = Options.Junctions});
				Target.Slot = FaultSlot;
				for (const uint32 FaultyNode : Faults.Nodes)
				{
					const NodeId Id{FaultyNode};
					const double Widest = Network->GetWidestWall(Id);
					const Vec2 Position = Network->FindNode(Id)->Position;
					const auto Joint = std::find_if(Unclipped.Nodes.begin(), Unclipped.Nodes.end(), [Id](const NodePiece& Piece) { return Piece.Node == Id; });
					const std::vector<Vec2> Marker = Joint != Unclipped.Nodes.end()
						? ClipToConvex(Joint->Polygon, MakeRegularPolygon(Position, FaultJointReach * Widest, 48))
						: MakeRegularPolygon(Position, 0.5 * Widest, 24);
					AppendPrism(Target, Marker, Style.Height + FaultJointRise);
				}
			}
		}

		Mesh.SetNumMaterials(SlotCount);
		SetSlotColor(Mesh, WallSlot, BaseMaterial, Style.Color);
		SetSlotColor(Mesh, FaultSlot, BaseMaterial, FaultColor);
		SetSlotColor(Mesh, HighlightSlot, BaseMaterial, CastleWallDrawing::HighlightColor);
		Mesh.SetMesh(MoveTemp(Walls));
		Mesh.SetVisibility(Network != nullptr);
	}
}
