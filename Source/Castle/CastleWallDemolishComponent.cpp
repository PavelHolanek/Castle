#include "CastleWallDemolishComponent.h"

#include "CastleCoreConversion.h"
#include "CastleCore/WallDemolition.h"
#include "CastlePlayerController.h"
#include "CastleWallBuildComponent.h"
#include "CastleWallSubsystem.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"

void UCastleWallDemolishComponent::CreateMappings(UInputMappingContext& Context)
{
	DemolishAction = NewObject<UInputAction>(this, TEXT("IA_DemolishWall"));
	Context.MapKey(DemolishAction, EKeys::LeftMouseButton);

	LeaveAction = NewObject<UInputAction>(this, TEXT("IA_LeaveDemolish"));
	Context.MapKey(LeaveAction, EKeys::RightMouseButton);
}

void UCastleWallDemolishComponent::BindInput(UEnhancedInputComponent& Input)
{
	Input.BindAction(DemolishAction, ETriggerEvent::Started, this, &UCastleWallDemolishComponent::Demolish);
	Input.BindAction(LeaveAction, ETriggerEvent::Started, this, &UCastleWallDemolishComponent::Leave);
}

void UCastleWallDemolishComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UCastleWallSubsystem* Walls = GetWalls())
	{
		Walls->SetHighlightedWall(FindTarget());
	}
	ShowInfo();
}

void UCastleWallDemolishComponent::OnDeactivated()
{
	if (UCastleWallSubsystem* Walls = GetWalls())
	{
		Walls->SetHighlightedWall({});
	}
}

Castle::Core::WallId UCastleWallDemolishComponent::FindTarget() const
{
	const UCastleWallSubsystem* Walls = GetWalls();
	FVector Cursor;
	double PickRadius = 0.0;
	if (!Walls || !GetCursorOnGround(Cursor, PickRadius))
	{
		return {};
	}

	const std::optional<Castle::Core::WallPoint> OnWall = Walls->GetNetwork().FindWallAt(CastleCoreConversion::ToCore(Cursor), PickRadius);
	return OnWall ? OnWall->Wall : Castle::Core::WallId{};
}

void UCastleWallDemolishComponent::Demolish()
{
	UCastleWallSubsystem* Walls = GetWalls();
	const Castle::Core::WallId Target = FindTarget();
	if (!Walls || !Target.IsValid())
	{
		return;
	}

	Walls->ModifyNetwork([this, Target](Castle::Core::WallNetwork& Network)
	{
		Castle::Core::DemolishWall(Network, Target, Limits);
	});
}

void UCastleWallDemolishComponent::Leave()
{
	if (ACastlePlayerController* Controller = GetOwner<ACastlePlayerController>())
	{
		Controller->SetActiveTool(Controller->GetWallBuild());
	}
}

void UCastleWallDemolishComponent::ShowInfo() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::White,
			TEXT("Demolish\nLeft click: remove the wall\nRight click or X: back to building"));
	}
}
