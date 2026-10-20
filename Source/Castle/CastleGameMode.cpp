#include "CastleGameMode.h"

#include "CastleCameraPawn.h"
#include "CastlePlayerController.h"

ACastleGameMode::ACastleGameMode()
{
	DefaultPawnClass = ACastleCameraPawn::StaticClass();
	PlayerControllerClass = ACastlePlayerController::StaticClass();
}
