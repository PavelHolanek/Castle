#include "CastleToolComponent.h"

#include "CastlePlayerController.h"
#include "CastleWallSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

namespace
{
	/** Above the camera's controls, so that the tool gets the mouse buttons first. */
	constexpr int32 ToolMappingPriority = 1;
}

UCastleToolComponent::UCastleToolComponent()
{
	// Only the active tool ticks; activating it switches the tick on.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCastleToolComponent::SetupInput(UEnhancedInputComponent& Input)
{
	if (!MappingContext)
	{
		MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Tool"));
		CreateMappings(*MappingContext);
	}
	BindInput(Input);
	ApplyMappingContext();
}

void UCastleToolComponent::Activate(bool bReset)
{
	Super::Activate(bReset);
	ApplyMappingContext();
}

void UCastleToolComponent::Deactivate()
{
	const bool bWasActive = IsActive();
	Super::Deactivate();
	ApplyMappingContext();
	if (bWasActive)
	{
		OnDeactivated();
	}
}

bool UCastleToolComponent::GetCursorOnGround(FVector& OutPoint, double& OutPickRadius) const
{
	const ACastlePlayerController* Controller = GetOwner<ACastlePlayerController>();
	return Controller && Controller->GetCursorOnGround(OutPoint, OutPickRadius);
}

UCastleWallSubsystem* UCastleToolComponent::GetWalls() const
{
	return GetWorld()->GetSubsystem<UCastleWallSubsystem>();
}

void UCastleToolComponent::ApplyMappingContext() const
{
	const APlayerController* Controller = GetOwner<APlayerController>();
	UEnhancedInputLocalPlayerSubsystem* Input = Controller
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer())
		: nullptr;
	if (!Input || !MappingContext)
	{
		return;
	}

	if (IsActive())
	{
		Input->AddMappingContext(MappingContext, ToolMappingPriority);
	}
	else
	{
		Input->RemoveMappingContext(MappingContext);
	}
}
