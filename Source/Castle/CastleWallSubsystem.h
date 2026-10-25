#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallNetwork.h"
#include "CastleWallViewMode.h"
#include "Subsystems/WorldSubsystem.h"
#include "CastleWallSubsystem.generated.h"

class ACastleWallNetworkActor;

/**
 * Owns the world's wall network, the authoritative model of what the player built.
 * Changes go through ModifyNetwork so that the drawing follows them.
 */
UCLASS()
class CASTLE_API UCastleWallSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	const Castle::Core::WallNetwork& GetNetwork() const { return Network; }

	/** Lets Edit change the network, then redraws it. Ends a preview. */
	void ModifyNetwork(TFunctionRef<void(Castle::Core::WallNetwork&)> Edit);

	/** Draws Preview in place of the network, e.g. while a wall is being placed, until ClearPreview or the next change. */
	void ShowPreview(const Castle::Core::WallNetwork& Preview);
	void ClearPreview();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModeChanged, ECastleWallViewMode /* ViewMode */);

	/** Fires whenever the view mode changes. */
	FOnViewModeChanged OnViewModeChanged;

	ECastleWallViewMode GetViewMode() const { return ViewMode; }
	void SetViewMode(ECastleWallViewMode InViewMode);

protected:
	/** Only worlds that are played have walls to build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void Draw(const Castle::Core::WallNetwork& Shown);

	Castle::Core::WallNetwork Network;
	bool bShowingPreview = false;
	/** What the player sees; shipping builds have no switch to change it. */
	ECastleWallViewMode ViewMode = ECastleWallViewMode::Walls3D;

	UPROPERTY(Transient)
	TObjectPtr<ACastleWallNetworkActor> NetworkActor;
};
