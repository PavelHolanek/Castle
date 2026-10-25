#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallNetwork.h"
#include "CastleWallMesh.h"
#include "CastleWallViewMode.h"
#include "GameFramework/Actor.h"
#include "CastleWallNetworkActor.generated.h"

class UDynamicMeshComponent;
class ULineBatchComponent;
class UMaterialInterface;

/**
 * Shows the world's wall network as solid walls or debug lines, depending on the view mode. Spawned by
 * UCastleWallSubsystem, which tells it when to redraw.
 */
UCLASS(NotPlaceable, Transient)
class CASTLE_API ACastleWallNetworkActor : public AActor
{
	GENERATED_BODY()

public:
	ACastleWallNetworkActor();

	void Redraw(const Castle::Core::WallNetwork& Network, ECastleWallViewMode ViewMode);

private:
	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<ULineBatchComponent> Lines;

	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<UDynamicMeshComponent> WallMesh;

	UPROPERTY(EditAnywhere, Category = "Walls")
	FCastleWallStyle Style;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;
};
