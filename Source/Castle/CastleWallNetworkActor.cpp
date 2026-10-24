#include "CastleWallNetworkActor.h"

#include "CastleWallDrawing.h"
#include "Components/LineBatchComponent.h"

ACastleWallNetworkActor::ACastleWallNetworkActor()
{
	// The lines are in world space, the component's transform doesn't move them.
	Lines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("Lines"));
	RootComponent = Lines;
}

void ACastleWallNetworkActor::Redraw(const Castle::Core::WallNetwork& Network)
{
	CastleWallDrawing::DrawNetwork(*Lines, Network, FTransform::Identity, FCastleWallDrawOptions());
}
