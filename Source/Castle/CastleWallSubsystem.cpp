#include "CastleWallSubsystem.h"

#include "CastleWallNetworkActor.h"
#include "Engine/World.h"

void UCastleWallSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	NetworkActor = InWorld.SpawnActor<ACastleWallNetworkActor>(SpawnParameters);
	Draw(Network);
}

void UCastleWallSubsystem::ModifyNetwork(TFunctionRef<void(Castle::Core::WallNetwork&)> Edit)
{
	Edit(Network);

	bShowingPreview = false;
	Draw(Network);
}

void UCastleWallSubsystem::ShowPreview(const Castle::Core::WallNetwork& Preview)
{
	bShowingPreview = true;
	Draw(Preview);
}

void UCastleWallSubsystem::ClearPreview()
{
	if (bShowingPreview)
	{
		bShowingPreview = false;
		Draw(Network);
	}
}

void UCastleWallSubsystem::SetViewMode(ECastleWallViewMode InViewMode)
{
	if (ViewMode == InViewMode)
	{
		return;
	}
	ViewMode = InViewMode;

	// A preview is shown anew every frame, the network has to be redrawn here.
	if (!bShowingPreview)
	{
		Draw(Network);
	}
	OnViewModeChanged.Broadcast(ViewMode);
}

bool UCastleWallSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCastleWallSubsystem::Draw(const Castle::Core::WallNetwork& Shown)
{
	if (NetworkActor)
	{
		NetworkActor->Redraw(Shown, ViewMode);
	}
}
