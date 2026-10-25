#include "CastleWallDebugActor.h"

#include "CastleCoreConversion.h"
#include "CastleWallDrawing.h"
#include "Components/BillboardComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FCastleDebugWall MakeDebugWall(int32 NodeA, int32 NodeB, float Width, float Bulge = 0.f)
	{
		FCastleDebugWall Wall;
		Wall.NodeA = NodeA;
		Wall.NodeB = NodeB;
		Wall.WidthLeft = 0.5f * Width;
		Wall.WidthRight = 0.5f * Width;
		Wall.Bulge = Bulge;
		return Wall;
	}

	FCastleDebugWall MakeAsymmetricDebugWall(int32 NodeA, int32 NodeB, float WidthLeft, float WidthRight)
	{
		FCastleDebugWall Wall = MakeDebugWall(NodeA, NodeB, 0.f);
		Wall.WidthLeft = WidthLeft;
		Wall.WidthRight = WidthRight;
		return Wall;
	}
}

ACastleWallDebugActor::ACastleWallDebugActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Lines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("Lines"));
	Lines->SetupAttachment(SceneRoot);

	// Relative to the actor, like the nodes, so the mesh moves with it.
	WallMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(SceneRoot);
	WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallMesh->SetTangentsType(EDynamicMeshComponentTangentsMode::NoTangents);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(CastleWallMesh::BaseMaterialPath);
	BaseMaterial = Material.Object;

#if WITH_EDITORONLY_DATA
	// Something to click on in the viewport, the lines themselves can't be selected.
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(SceneRoot);
	}
#endif

	Nodes = {
		// Two walls meeting at an angle, as in segment.PNG.
		FVector(-560.0, 600.0, 0.0),
		FVector(0.0, 0.0, 0.0),
		FVector(780.0, -230.0, 0.0),
		// A thin wall joining a thick one (T).
		FVector(-700.0, 1500.0, 0.0),
		FVector(0.0, 1500.0, 0.0),
		FVector(700.0, 1500.0, 0.0),
		FVector(0.0, 2300.0, 0.0),
		// An arc.
		FVector(1500.0, -300.0, 0.0),
		FVector(1500.0, 1100.0, 0.0),
		// Three walls of different widths meeting at 125, 105 and 130 degrees; one of them ends at the node.
		FVector(3000.0, 0.0, 0.0),
		FVector(3705.0, 257.0, 0.0),
		FVector(2427.0, 402.0, 0.0),
		FVector(2726.0, -752.0, 0.0),
		// Two collinear walls of different lengths.
		FVector(2500.0, 1500.0, 0.0),
		FVector(2900.0, 1500.0, 0.0),
		FVector(3900.0, 1500.0, 0.0),
		// Two collinear walls of different widths.
		FVector(2500.0, 2300.0, 0.0),
		FVector(3100.0, 2300.0, 0.0),
		FVector(3900.0, 2300.0, 0.0),
		// Two walls at an acute angle of 60 degrees.
		FVector(4550.0, 221.0, 0.0),
		FVector(5000.0, 1000.0, 0.0),
		FVector(5450.0, 221.0, 0.0),
		// Four walls of different widths at right angles (X); one of them ends at the node.
		FVector(6800.0, 0.0, 0.0),
		FVector(7600.0, 0.0, 0.0),
		FVector(6800.0, 800.0, 0.0),
		FVector(6000.0, 0.0, 0.0),
		FVector(6800.0, -800.0, 0.0),
		// Two walls at 40 degrees, each with the reference line off center.
		FVector(6492.0, 1454.0, 0.0),
		FVector(6800.0, 2300.0, 0.0),
		FVector(7108.0, 1454.0, 0.0),
		// A wide and a thin wall at 135 degrees: the thin one's side passes behind the wide one's end.
		FVector(100.0, 4000.0, 0.0),
		FVector(1000.0, 4000.0, 0.0),
		FVector(1636.0, 4636.0, 0.0),
		// Different widths at 110 degrees: the sides still meet on the wide wall.
		FVector(2100.0, 4000.0, 0.0),
		FVector(3000.0, 4000.0, 0.0),
		FVector(3308.0, 4846.0, 0.0),
		// The same widths at 160 degrees: the step is back, and longer.
		FVector(4100.0, 4000.0, 0.0),
		FVector(5000.0, 4000.0, 0.0),
		FVector(5846.0, 4308.0, 0.0),
		// The same widths at 170 degrees: the extension would be too long and gets beveled.
		FVector(100.0, 6000.0, 0.0),
		FVector(1000.0, 6000.0, 0.0),
		FVector(1886.3, 6156.3, 0.0),
		// A thin wall too short to reach: the extension ends at the middle of its side.
		FVector(2100.0, 6000.0, 0.0),
		FVector(3000.0, 6000.0, 0.0),
		FVector(3075.0, 6129.9, 0.0),
		// Outer side at 225 degrees with widths 200 and 141.4 (cos 45 = 141.4 / 200): the wider wall's corner
		// lies on the thinner wall's side line. The inner sides are equally wide.
		FVector(100.0, 8000.0, 0.0),
		FVector(1000.0, 8000.0, 0.0),
		FVector(1636.4, 8636.4, 0.0),
		// An L of different widths with a sharp outer corner.
		FVector(2100.0, 8000.0, 0.0),
		FVector(3000.0, 8000.0, 0.0),
		FVector(3000.0, 8900.0, 0.0),
		// The same L with a beveled outer corner (see BeveledNodes).
		FVector(4100.0, 8000.0, 0.0),
		FVector(5000.0, 8000.0, 0.0),
		FVector(5000.0, 8900.0, 0.0),
	};

	Walls = {
		MakeDebugWall(0, 1, 200.f),
		MakeDebugWall(1, 2, 200.f),
		MakeDebugWall(3, 4, 300.f),
		MakeDebugWall(4, 5, 300.f),
		MakeDebugWall(4, 6, 120.f),
		MakeDebugWall(7, 8, 200.f, 0.5f),
		MakeDebugWall(9, 10, 200.f),
		MakeDebugWall(9, 11, 160.f),
		MakeDebugWall(12, 9, 240.f),
		MakeDebugWall(13, 14, 200.f),
		MakeDebugWall(14, 15, 200.f),
		MakeDebugWall(16, 17, 300.f),
		MakeDebugWall(17, 18, 160.f),
		MakeDebugWall(19, 20, 200.f),
		MakeDebugWall(20, 21, 200.f),
		MakeDebugWall(22, 23, 200.f),
		MakeDebugWall(24, 22, 300.f),
		MakeDebugWall(22, 25, 200.f),
		MakeDebugWall(22, 26, 120.f),
		MakeAsymmetricDebugWall(27, 28, 60.f, 240.f),
		MakeAsymmetricDebugWall(28, 29, 200.f, 40.f),
		MakeDebugWall(30, 31, 400.f),
		MakeDebugWall(31, 32, 80.f),
		MakeDebugWall(33, 34, 300.f),
		MakeDebugWall(34, 35, 160.f),
		MakeDebugWall(36, 37, 300.f),
		MakeDebugWall(37, 38, 160.f),
		MakeDebugWall(39, 40, 300.f),
		MakeDebugWall(40, 41, 160.f),
		MakeDebugWall(42, 43, 400.f),
		MakeDebugWall(43, 44, 80.f),
		MakeAsymmetricDebugWall(45, 46, 200.f, 100.f),
		MakeAsymmetricDebugWall(46, 47, 141.42136f, 100.f),
		MakeDebugWall(48, 49, 300.f),
		MakeDebugWall(49, 50, 120.f),
		MakeDebugWall(51, 52, 300.f),
		MakeDebugWall(52, 53, 120.f),
	};

	BeveledNodes = {52};
}

void ACastleWallDebugActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Redraw();
}

void ACastleWallDebugActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	// The batched lines aren't saved or duplicated for PIE, so a loaded actor has to draw them again.
	Redraw();
}

void ACastleWallDebugActor::Redraw()
{
	using namespace Castle::Core;

	if (!Lines || !WallMesh)
	{
		return;
	}

	WallNetwork Network;
	TArray<NodeId> NodeIds;
	for (const FVector& Position : Nodes)
	{
		NodeIds.Add(Network.AddNode(CastleCoreConversion::ToCore(Position)));
	}
	for (const int32 NodeIndex : BeveledNodes)
	{
		if (NodeIds.IsValidIndex(NodeIndex))
		{
			Network.SetCornerStyle(NodeIds[NodeIndex], ECornerStyle::Beveled);
		}
	}

	// Nodes keep their index as id; walls with an invalid node are skipped, so they need a lookup.
	TMap<uint32, int32> WallIndexById;
	for (int32 WallIndex = 0; WallIndex < Walls.Num(); ++WallIndex)
	{
		const FCastleDebugWall& DebugWall = Walls[WallIndex];
		if (!NodeIds.IsValidIndex(DebugWall.NodeA) || !NodeIds.IsValidIndex(DebugWall.NodeB))
		{
			continue;
		}

		const WallId Id = Network.AddWall(NodeIds[DebugWall.NodeA], NodeIds[DebugWall.NodeB], DebugWall.WidthLeft, DebugWall.WidthRight, DebugWall.Bulge);
		if (Id.IsValid())
		{
			WallIndexById.Add(Id.Value, WallIndex);
		}
	}

	FCastleWallDrawOptions Options;
	Options.ArcStep = ArcStep;
	Options.LineThickness = LineThickness;
	Options.NodeSize = NodeSize;
	Options.DrawHeight = DrawHeight;
	Options.ViewMode = ViewMode;
	Options.Junctions.MaxExtensionFactor = MaxExtensionFactor;
	Options.WallLabel = [&WallIndexById](WallId Id) { return WallIndexById.FindRef(Id.Value); };

	FCastleWallDrawReport Report;
	CastleWallDrawing::DrawNetwork(*Lines, Network, GetActorTransform(), Options, &Report);
	LimitViolations = MoveTemp(Report.LimitViolations);
	JunctionCorners = MoveTemp(Report.JunctionCorners);
	FloorPlanPieces = MoveTemp(Report.FloorPlanPieces);

	CastleWallMesh::ShowWalls(*WallMesh, ViewMode == ECastleWallViewMode::Walls3D ? &Network : nullptr, Options, Style, BaseMaterial);
}
