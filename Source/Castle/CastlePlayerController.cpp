#include "CastlePlayerController.h"

ACastlePlayerController::ACastlePlayerController()
{
	bShowMouseCursor = true;
}

void ACastlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}
