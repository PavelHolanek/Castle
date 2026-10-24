#include "CastleWallBuildComponent.h"

#include "CastleCoreConversion.h"
#include "CastlePlayerController.h"
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
	FCastleWallType MakeWallType(const TCHAR* Name, float Width)
	{
		FCastleWallType Type;
		Type.Name = Name;
		Type.Width = Width;
		return Type;
	}
}

UCastleWallBuildComponent::UCastleWallBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	WallTypes = {
		MakeWallType(TEXT("Thin wall"), 60.f),
		MakeWallType(TEXT("Wall"), 150.f),
		MakeWallType(TEXT("Curtain wall"), 300.f),
	};
}

void UCastleWallBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	SelectWallType(SelectedWallType);
}

void UCastleWallBuildComponent::SetupInput(UEnhancedInputComponent& Input)
{
	if (!MappingContext)
	{
		CreateInputMappings();
	}

	if (const APlayerController* Controller = GetOwner<APlayerController>())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 1);
		}
	}

	Input.BindAction(PlaceAction, ETriggerEvent::Started, this, &UCastleWallBuildComponent::Place);
	Input.BindAction(CancelAction, ETriggerEvent::Started, this, &UCastleWallBuildComponent::CancelPlacement);
	for (int32 Index = 0; Index < SelectTypeActions.Num(); ++Index)
	{
		Input.BindAction(SelectTypeActions[Index], ETriggerEvent::Started, this, &UCastleWallBuildComponent::SelectWallType, Index);
	}
}

void UCastleWallBuildComponent::CreateInputMappings()
{
	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_WallBuild"));

	PlaceAction = NewObject<UInputAction>(this, TEXT("IA_PlaceWall"));
	MappingContext->MapKey(PlaceAction, EKeys::LeftMouseButton);

	CancelAction = NewObject<UInputAction>(this, TEXT("IA_CancelWall"));
	MappingContext->MapKey(CancelAction, EKeys::RightMouseButton);

	const FKey TypeKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
	for (int32 Index = 0; Index < FMath::Min(WallTypes.Num(), static_cast<int32>(UE_ARRAY_COUNT(TypeKeys))); ++Index)
	{
		UInputAction* Action = NewObject<UInputAction>(this, FName(TEXT("IA_SelectWallType"), Index + 1));
		MappingContext->MapKey(Action, TypeKeys[Index]);
		SelectTypeActions.Add(Action);
	}
}

void UCastleWallBuildComponent::SelectWallType(int32 Index)
{
	if (WallTypes.IsValidIndex(Index))
	{
		SelectedWallType = Index;
		const double HalfWidth = 0.5 * WallTypes[Index].Width;
		Placement.SetWidths(HalfWidth, HalfWidth);
		OnWallTypeSelected.Broadcast(Index);
	}
}

void UCastleWallBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACastlePlayerController* Controller = GetOwner<ACastlePlayerController>();
	const UCastleWallSubsystem* Walls = GetWorld()->GetSubsystem<UCastleWallSubsystem>();
	FVector Cursor;
	double PickRadius = 0.0;
	if (!Controller || !Walls || !Controller->GetCursorOnGround(Cursor, PickRadius))
	{
		return;
	}

	Placement.Hover(Walls->GetNetwork(), CastleCoreConversion::ToCore(Cursor), PickRadius);
	UpdatePreview();
	ShowInfo();

	// Ring around the node the cursor snaps to.
	if (const Castle::Core::Node* Snapped = Walls->GetNetwork().FindNode(Placement.GetSnapNode()))
	{
		DrawDebugCircle(GetWorld(), CastleCoreConversion::ToUnreal(Snapped->Position, Cursor.Z + 3.0), PickRadius, 32, FColor::Cyan,
			false, -1.f, SDPG_Foreground, 4.f, FVector::XAxisVector, FVector::YAxisVector, false);
	}
}

void UCastleWallBuildComponent::Place()
{
	const ACastlePlayerController* Controller = GetOwner<ACastlePlayerController>();
	UCastleWallSubsystem* Walls = GetWorld()->GetSubsystem<UCastleWallSubsystem>();
	FVector Cursor;
	double PickRadius = 0.0;
	if (!Controller || !Walls || !Controller->GetCursorOnGround(Cursor, PickRadius))
	{
		return;
	}

	Walls->ModifyNetwork([this, &Cursor, PickRadius](Castle::Core::WallNetwork& Network)
	{
		Placement.Click(Network, CastleCoreConversion::ToCore(Cursor), PickRadius);
	});
	UpdatePreview();
}

void UCastleWallBuildComponent::CancelPlacement()
{
	Placement.Cancel();
	UpdatePreview();
}

void UCastleWallBuildComponent::UpdatePreview()
{
	UCastleWallSubsystem* Walls = GetWorld()->GetSubsystem<UCastleWallSubsystem>();
	if (!Walls)
	{
		return;
	}

	if (const std::optional<Castle::Core::WallNetwork>& Preview = Placement.GetPreview())
	{
		Walls->ShowPreview(*Preview);
	}
	else
	{
		Walls->ClearPreview();
	}
}

void UCastleWallBuildComponent::ShowInfo() const
{
	if (!GEngine || !WallTypes.IsValidIndex(SelectedWallType))
	{
		return;
	}

	const FCastleWallType& Type = WallTypes[SelectedWallType];
	FString Info = FString::Printf(TEXT("Wall type %d: %s, %.0f cm (keys 1-%d)"), SelectedWallType + 1, *Type.Name, Type.Width, SelectTypeActions.Num());
	Info += Placement.GetState() == Castle::Core::WallPlacement::EState::PickingStart
		? TEXT("\nLeft click: start a wall")
		: TEXT("\nLeft click: place, right click: stop");
	FColor Color = FColor::White;

	if (const std::optional<Castle::Core::WallNetwork>& Preview = Placement.GetPreview())
	{
		const std::optional<Castle::Core::WallCurve> Reference = Preview->GetReferenceCurve(Placement.GetPreviewWall());
		Info += FString::Printf(TEXT("\nLength %.1f cm"), Reference ? Reference->GetLength() : 0.0);

		const std::vector<Castle::Core::ELimitViolation>& Violations = Placement.GetPreviewViolations();
		if (!Violations.empty())
		{
			TArray<FString> Names;
			for (const Castle::Core::ELimitViolation Violation : Violations)
			{
				Names.Add(UTF8_TO_TCHAR(Castle::Core::ToString(Violation)));
			}
			Info += TEXT("\nOutside limits: ") + FString::Join(Names, TEXT(", "));
			Color = FColor::Magenta;
		}
	}

	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, Color, Info);
}
