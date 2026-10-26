#include "CastleWallBuildComponent.h"

#include "CastleCoreConversion.h"
#include "CastleWallSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
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
	WallTypes = {
		MakeWallType(TEXT("Thin wall"), 60.f),
		MakeWallType(TEXT("Wall"), 150.f),
		MakeWallType(TEXT("Curtain wall"), 300.f),
	};
}

void UCastleWallBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Placement.SetSnapWidthFactor(SnapWidthFactor);
	SelectWallType(SelectedWallType);
}

void UCastleWallBuildComponent::CreateMappings(UInputMappingContext& Context)
{
	PlaceAction = NewObject<UInputAction>(this, TEXT("IA_PlaceWall"));
	Context.MapKey(PlaceAction, EKeys::LeftMouseButton);

	CancelAction = NewObject<UInputAction>(this, TEXT("IA_CancelWall"));
	Context.MapKey(CancelAction, EKeys::RightMouseButton);
}

void UCastleWallBuildComponent::BindInput(UEnhancedInputComponent& Input)
{
	Input.BindAction(PlaceAction, ETriggerEvent::Started, this, &UCastleWallBuildComponent::Place);
	Input.BindAction(CancelAction, ETriggerEvent::Started, this, &UCastleWallBuildComponent::CancelPlacement);
}

void UCastleWallBuildComponent::OnDeactivated()
{
	CancelPlacement();
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

	const UCastleWallSubsystem* Walls = GetWalls();
	FVector Cursor;
	double PickRadius = 0.0;
	if (!Walls || !GetCursorOnGround(Cursor, PickRadius))
	{
		return;
	}

	Placement.Hover(Walls->GetNetwork(), CastleCoreConversion::ToCore(Cursor), PickRadius);
	UpdatePreview();
	ShowInfo();

	// Ring around the node the cursor snaps to, or a line across the wall where it would be split.
	const Castle::Core::WallNetwork& Network = Walls->GetNetwork();
	const double MarkHeight = Cursor.Z + 3.0;
	if (const Castle::Core::Node* Snapped = Network.FindNode(Placement.GetSnapNode()))
	{
		const double SnapRadius = Placement.GetSnapRadius(Network, Placement.GetSnapNode(), PickRadius);
		DrawDebugCircle(GetWorld(), CastleCoreConversion::ToUnreal(Snapped->Position, MarkHeight), SnapRadius, 32, FColor::Cyan,
			false, -1.f, SDPG_Foreground, 4.f, FVector::XAxisVector, FVector::YAxisVector, false);
	}
	else if (const std::optional<Castle::Core::WallPoint>& OnWall = Placement.GetSnapWall())
	{
		const Castle::Core::Wall& Split = *Network.FindWall(OnWall->Wall);
		const Castle::Core::Vec2 Left = Castle::Core::PerpLeft(Network.GetReferenceCurve(OnWall->Wall)->TangentAt(OnWall->T));
		DrawDebugLine(GetWorld(), CastleCoreConversion::ToUnreal(OnWall->Position + Left * Split.WidthLeft, MarkHeight),
			CastleCoreConversion::ToUnreal(OnWall->Position - Left * Split.WidthRight, MarkHeight), FColor::Cyan, false, -1.f,
			SDPG_Foreground, 4.f);
	}
}

void UCastleWallBuildComponent::Place()
{
	UCastleWallSubsystem* Walls = GetWalls();
	FVector Cursor;
	double PickRadius = 0.0;
	if (!Walls || !GetCursorOnGround(Cursor, PickRadius))
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
	UCastleWallSubsystem* Walls = GetWalls();
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
	FString Info = FString::Printf(TEXT("Wall type %d: %s, %.0f cm (keys 1-%d)"), SelectedWallType + 1, *Type.Name, Type.Width,
		FMath::Min(WallTypes.Num(), 9));
	Info += Placement.GetState() == Castle::Core::WallPlacement::EState::PickingStart
		? TEXT("\nLeft click: start a wall")
		: TEXT("\nLeft click: place, right click: stop");
	Info += TEXT("\nX: demolish");
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
