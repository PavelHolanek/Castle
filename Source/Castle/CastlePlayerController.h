#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CastlePlayerController.generated.h"

/** Player controller for the strategy view: the mouse cursor stays visible. */
UCLASS()
class CASTLE_API ACastlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACastlePlayerController();

protected:
	virtual void BeginPlay() override;
};
