#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallNetwork.h"
#include "GameFramework/Actor.h"
#include "CastleWallNetworkActor.generated.h"

class ULineBatchComponent;

/** Draws the world's wall network with debug lines. Spawned by UCastleWallSubsystem, which tells it when to redraw. */
UCLASS(NotPlaceable, Transient)
class CASTLE_API ACastleWallNetworkActor : public AActor
{
	GENERATED_BODY()

public:
	ACastleWallNetworkActor();

	void Redraw(const Castle::Core::WallNetwork& Network);

private:
	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<ULineBatchComponent> Lines;
};
