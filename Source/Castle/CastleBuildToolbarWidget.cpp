#include "CastleBuildToolbarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CastlePlayerController.h"
#include "CastleWallBuildComponent.h"
#include "CastleWallDemolishComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor IdleButtonColor(0.08f, 0.08f, 0.08f);
	const FLinearColor SelectedButtonColor(1.f, 0.45f, 0.05f);
}

UCastleToolbarButton::UCastleToolbarButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Otherwise a clicked button keeps the keyboard focus.
	InitIsFocusable(false);
}

void UCastleToolbarButton::Setup(int32 InIndex, FOnChosen InOnChosen)
{
	Index = InIndex;
	OnChosen = MoveTemp(InOnChosen);
	OnClicked.AddUniqueDynamic(this, &UCastleToolbarButton::HandleClicked);
}

void UCastleToolbarButton::SetSelected(bool bSelected)
{
	SetBackgroundColor(bSelected ? SelectedButtonColor : IdleButtonColor);
}

void UCastleToolbarButton::HandleClicked()
{
	OnChosen.ExecuteIfBound(Index);
}

void UCastleBuildToolbarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The root covers the screen but lets clicks through to the game, except on the toolbar itself.
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Background->SetPadding(FMargin(8.f));

	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.5f, 1.f));
	BackgroundSlot->SetAlignment(FVector2D(0.5, 1.0));
	BackgroundSlot->SetPosition(FVector2D(0.0, -16.0));
	BackgroundSlot->SetAutoSize(true);

	ButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	Background->SetContent(ButtonBox);
}

void UCastleBuildToolbarWidget::NativeDestruct()
{
	Unbind();

	Super::NativeDestruct();
}

void UCastleBuildToolbarWidget::SetController(ACastlePlayerController* InController)
{
	Unbind();
	Controller = InController;
	ButtonBox->ClearChildren();
	TypeButtons.Reset();
	DemolishButton = nullptr;
	if (!InController)
	{
		return;
	}

	const auto AddButton = [this](int32 Index, const FString& Text, UCastleToolbarButton::FOnChosen OnChosen, const FMargin& ButtonPadding)
	{
		UCastleToolbarButton* Button = WidgetTree->ConstructWidget<UCastleToolbarButton>(UCastleToolbarButton::StaticClass());
		Button->Setup(Index, MoveTemp(OnChosen));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetJustification(ETextJustify::Center);
		Label->SetFontSize(14.f);
		Button->SetContent(Label);

		ButtonBox->AddChildToHorizontalBox(Button)->SetPadding(ButtonPadding);
		return Button;
	};

	UCastleWallBuildComponent* Build = InController->GetWallBuild();
	const TArray<FCastleWallType>& Types = Build->GetWallTypes();
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		TypeButtons.Add(AddButton(Index, FString::Printf(TEXT("%d  %s\n%.0f cm"), Index + 1, *Types[Index].Name, Types[Index].Width),
			UCastleToolbarButton::FOnChosen::CreateUObject(this, &UCastleBuildToolbarWidget::ChooseWallType), FMargin(4.f, 0.f)));
	}

	// Set apart from the wall types.
	DemolishButton = AddButton(INDEX_NONE, TEXT("X  Demolish"),
		UCastleToolbarButton::FOnChosen::CreateUObject(this, &UCastleBuildToolbarWidget::ChooseDemolish), FMargin(20.f, 0.f, 4.f, 0.f));

	WallTypeHandle = Build->OnWallTypeSelected.AddWeakLambda(this, [this](int32) { Refresh(); });
	ToolHandle = InController->OnActiveToolChanged.AddWeakLambda(this, [this](UCastleToolComponent*) { Refresh(); });
	Refresh();
}

void UCastleBuildToolbarWidget::Unbind()
{
	if (ACastlePlayerController* Previous = Controller.Get())
	{
		Previous->GetWallBuild()->OnWallTypeSelected.Remove(WallTypeHandle);
		Previous->OnActiveToolChanged.Remove(ToolHandle);
	}
	Controller = nullptr;
}

void UCastleBuildToolbarWidget::Refresh()
{
	const ACastlePlayerController* Owner = Controller.Get();
	if (!Owner)
	{
		return;
	}

	const UCastleWallBuildComponent* Build = Owner->GetWallBuild();
	const bool bBuilding = Owner->GetActiveTool() == Build;
	for (int32 Index = 0; Index < TypeButtons.Num(); ++Index)
	{
		TypeButtons[Index]->SetSelected(bBuilding && Index == Build->GetSelectedWallType());
	}
	DemolishButton->SetSelected(Owner->GetActiveTool() == Owner->GetWallDemolish());
}

void UCastleBuildToolbarWidget::ChooseWallType(int32 Index)
{
	if (ACastlePlayerController* Owner = Controller.Get())
	{
		Owner->SelectWallType(Index);
	}
}

void UCastleBuildToolbarWidget::ChooseDemolish(int32 /* Index */)
{
	if (ACastlePlayerController* Owner = Controller.Get())
	{
		Owner->SetActiveTool(Owner->GetWallDemolish());
	}
}
