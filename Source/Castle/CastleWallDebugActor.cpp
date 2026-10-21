#include "CastleWallDebugActor.h"

#include "CastleCore/WallNetwork.h"
#include "CastleCore/WallOutline.h"
#include "Components/BillboardComponent.h"
#include "Components/LineBatchComponent.h"

namespace
{
	// The core plane is right-handed while Unreal's XY plane seen from above is mirrored, so Y flips both ways.
	Castle::Core::Vec2 ToCore(const FVector& Local)
	{
		return {Local.X, -Local.Y};
	}

	FVector ToLocal(Castle::Core::Vec2 Point, double Z)
	{
		return FVector(Point.X, -Point.Y, Z);
	}

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
}

ACastleWallDebugActor::ACastleWallDebugActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Lines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("Lines"));
	Lines->SetupAttachment(SceneRoot);

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
	};

	Walls = {
		MakeDebugWall(0, 1, 200.f),
		MakeDebugWall(1, 2, 200.f),
		MakeDebugWall(3, 4, 300.f),
		MakeDebugWall(4, 5, 300.f),
		MakeDebugWall(4, 6, 120.f),
		MakeDebugWall(7, 8, 200.f, 0.5f),
	};
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

	if (!Lines)
	{
		return;
	}
	Lines->Flush();

	const FTransform& ActorTransform = GetActorTransform();
	const uint8 DepthPriority = SDPG_Foreground;
	const double ArcStepRadians = FMath::DegreesToRadians(static_cast<double>(ArcStep));

	TArray<FBatchedLine> Batch;
	const auto AddCurve = [&](const WallCurve& Curve, const FLinearColor& Color)
	{
		const std::vector<Vec2> Points = Curve.Tessellate(ArcStepRadians);
		for (size_t Index = 1; Index < Points.size(); ++Index)
		{
			Batch.Emplace(
				ActorTransform.TransformPosition(ToLocal(Points[Index - 1], DrawHeight)),
				ActorTransform.TransformPosition(ToLocal(Points[Index], DrawHeight)),
				Color, 0.f, LineThickness, DepthPriority);
		}
	};

	WallNetwork Network;
	TArray<NodeId> NodeIds;
	for (const FVector& Position : Nodes)
	{
		NodeIds.Add(Network.AddNode(ToCore(Position)));
		Lines->DrawPoint(ActorTransform.TransformPosition(FVector(Position.X, Position.Y, DrawHeight)), FLinearColor::Green, NodeSize, DepthPriority);
	}

	for (const FCastleDebugWall& DebugWall : Walls)
	{
		if (!NodeIds.IsValidIndex(DebugWall.NodeA) || !NodeIds.IsValidIndex(DebugWall.NodeB))
		{
			continue;
		}

		const WallId Id = Network.AddWall(NodeIds[DebugWall.NodeA], NodeIds[DebugWall.NodeB], DebugWall.WidthLeft, DebugWall.WidthRight, DebugWall.Bulge);
		const std::optional<RawWallOutline> Outline = BuildRawOutline(Network, Id);
		if (!Outline)
		{
			continue;
		}

		AddCurve(Outline->Reference, FLinearColor::Black);
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

	Lines->DrawLines(Batch);
}
