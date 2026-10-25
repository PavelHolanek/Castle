#include "CastleWallNetworkActor.h"

#include "CastleWallDrawing.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACastleWallNetworkActor::ACastleWallNetworkActor()
{
	// The lines are in world space, the component's transform doesn't move them.
	Lines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("Lines"));
	RootComponent = Lines;

	WallMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(Lines);
	WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallMesh->SetTangentsType(EDynamicMeshComponentTangentsMode::NoTangents);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(CastleWallMesh::BaseMaterialPath);
	BaseMaterial = Material.Object;
}

void ACastleWallNetworkActor::Redraw(const Castle::Core::WallNetwork& Network, ECastleWallViewMode ViewMode)
{
	FCastleWallDrawOptions Options;
	Options.ViewMode = ViewMode;
	CastleWallDrawing::DrawNetwork(*Lines, Network, FTransform::Identity, Options);

	CastleWallMesh::ShowWalls(*WallMesh, ViewMode == ECastleWallViewMode::Walls3D ? &Network : nullptr, Options, Style, BaseMaterial);
}
