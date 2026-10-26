#include "CastlePlayerController.h"

#include "CastleBuildToolbarWidget.h"
#include "CastleCoreConversion.h"
#include "CastleViewModeWidget.h"
#include "CastleWallBuildComponent.h"
#include "CastleWallDemolishComponent.h"
#include "CastleWallSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	/** Where the ray hits the horizontal plane at Height. False when it runs along the plane or away from it. */
	bool IntersectGround(const FVector& Origin, const FVector& Direction, double Height, FVector& OutPoint)
	{
		if (FMath::IsNearlyZero(Direction.Z))
		{
			return false;
		}

		const double Along = (Height - Origin.Z) / Direction.Z;
		if (Along < 0.0)
		{
			return false;
		}
		OutPoint = Origin + Direction * Along;
		return true;
	}
}

ACastlePlayerController::ACastlePlayerController()
{
	bShowMouseCursor = true;

	WallBuild = CreateDefaultSubobject<UCastleWallBuildComponent>(TEXT("WallBuild"));
	WallDemolish = CreateDefaultSubobject<UCastleWallDemolishComponent>(TEXT("WallDemolish"));
}

void ACastlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	SetActiveTool(WallBuild);

	if (IsLocalController())
	{
		Toolbar = CreateWidget<UCastleBuildToolbarWidget>(this, UCastleBuildToolbarWidget::StaticClass());
		Toolbar->SetController(this);
		Toolbar->AddToViewport();

#if !UE_BUILD_SHIPPING
		ViewModeSwitch = CreateWidget<UCastleViewModeWidget>(this, UCastleViewModeWidget::StaticClass());
		ViewModeSwitch->AddToViewport();
#endif
	}
}

void ACastlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent& Input = *CastChecked<UEnhancedInputComponent>(InputComponent);
	WallBuild->SetupInput(Input);
	WallDemolish->SetupInput(Input);

	if (!ToolMappingContext)
	{
		CreateToolMappings();
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(ToolMappingContext, 1);
	}

	for (int32 Index = 0; Index < SelectWallTypeActions.Num(); ++Index)
	{
		Input.BindAction(SelectWallTypeActions[Index], ETriggerEvent::Started, this, &ACastlePlayerController::SelectWallType, Index);
	}
	Input.BindAction(ToggleDemolishAction, ETriggerEvent::Started, this, &ACastlePlayerController::ToggleDemolish);
}

void ACastlePlayerController::CreateToolMappings()
{
	ToolMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Tools"));

	const FKey TypeKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
	const int32 TypeCount = FMath::Min(WallBuild->GetWallTypes().Num(), static_cast<int32>(UE_ARRAY_COUNT(TypeKeys)));
	for (int32 Index = 0; Index < TypeCount; ++Index)
	{
		UInputAction* Action = NewObject<UInputAction>(this, FName(TEXT("IA_SelectWallType"), Index + 1));
		ToolMappingContext->MapKey(Action, TypeKeys[Index]);
		SelectWallTypeActions.Add(Action);
	}

	ToggleDemolishAction = NewObject<UInputAction>(this, TEXT("IA_ToggleDemolish"));
	ToolMappingContext->MapKey(ToggleDemolishAction, EKeys::X);
}

void ACastlePlayerController::SetActiveTool(UCastleToolComponent* Tool)
{
	if (Tool == ActiveTool)
	{
		return;
	}

	if (ActiveTool)
	{
		ActiveTool->Deactivate();
	}
	ActiveTool = Tool;
	if (ActiveTool)
	{
		ActiveTool->Activate();
	}
	OnActiveToolChanged.Broadcast(ActiveTool);
}

void ACastlePlayerController::SelectWallType(int32 Index)
{
	WallBuild->SelectWallType(Index);
	SetActiveTool(WallBuild);
}

void ACastlePlayerController::ToggleDemolish()
{
	if (ActiveTool == WallDemolish)
	{
		SetActiveTool(WallBuild);
	}
	else
	{
		SetActiveTool(WallDemolish);
	}
}

void ACastlePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FVector Point;
	double PickRadius = 0.0;
	if (!GetCursorOnGround(Point, PickRadius))
	{
		return;
	}

	// A circle the size of the pick radius, lying on the ground.
	DrawDebugCircle(GetWorld(), Point + FVector(0.0, 0.0, 2.0), PickRadius, 32, FColor::Yellow, false, -1.f, SDPG_Foreground, 2.f,
		FVector::XAxisVector, FVector::YAxisVector, false);

	if (bShowCursorInfo && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::Yellow,
			FString::Printf(TEXT("Cursor: X %.1f, Y %.1f (pick radius %.1f)"), Point.X, Point.Y, PickRadius));
	}
}

bool ACastlePlayerController::GetCursorOnGround(FVector& OutPoint, double& OutPickRadius) const
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	FVector Origin;
	FVector Direction;
	if (!GetMousePosition(MouseX, MouseY)
		|| !DeprojectScreenPositionToWorld(MouseX, MouseY, Origin, Direction)
		|| !IntersectGround(Origin, Direction, GroundHeight, OutPoint))
	{
		return false;
	}

	// The same number of pixels covers more ground the farther away it is, so measure it right at the cursor.
	FVector OffsetPoint;
	if (DeprojectScreenPositionToWorld(MouseX + PickRadiusPixels, MouseY, Origin, Direction)
		&& IntersectGround(Origin, Direction, GroundHeight, OffsetPoint))
	{
		OutPickRadius = FVector::Dist(OutPoint, OffsetPoint);
	}
	else
	{
		OutPickRadius = 0.0;
	}
	return true;
}

void ACastlePlayerController::CastleAddWall(float StartX, float StartY, float EndX, float EndY, float Width)
{
	using namespace Castle::Core;

	UCastleWallSubsystem* Walls = GetWorld()->GetSubsystem<UCastleWallSubsystem>();
	if (!Walls)
	{
		return;
	}

	Walls->ModifyNetwork([&](WallNetwork& Network)
	{
		const auto FindOrAddNode = [&Network](float X, float Y)
		{
			const Vec2 Position = CastleCoreConversion::ToCore(FVector(X, Y, 0.0));
			const NodeId Existing = Network.FindNearestNode(Position, 1.0);
			return Existing.IsValid() ? Existing : Network.AddNode(Position);
		};

		const NodeId Start = FindOrAddNode(StartX, StartY);
		const NodeId End = FindOrAddNode(EndX, EndY);
		Network.AddWall(Start, End, 0.5 * Width, 0.5 * Width);
	});
}

void ACastlePlayerController::CastleClearWalls()
{
	if (UCastleWallSubsystem* Walls = GetWorld()->GetSubsystem<UCastleWallSubsystem>())
	{
		Walls->ModifyNetwork([](Castle::Core::WallNetwork& Network)
		{
			Network = Castle::Core::WallNetwork();
		});
	}
}
