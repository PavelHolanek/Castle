#include "CastleViewModeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CastleBuildToolbarWidget.h"
#include "CastleWallSubsystem.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UCastleViewModeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The root covers the screen but lets clicks through to the game, except on the switch itself.
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Background->SetPadding(FMargin(6.f));

	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(1.f, 0.f));
	BackgroundSlot->SetAlignment(FVector2D(1.0, 0.0));
	BackgroundSlot->SetPosition(FVector2D(-16.0, 16.0));
	BackgroundSlot->SetAutoSize(true);

	UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	Background->SetContent(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("View")));
	Title->SetFontSize(12.f);
	UHorizontalBoxSlot* TitleSlot = Box->AddChildToHorizontalBox(Title);
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleSlot->SetPadding(FMargin(4.f, 0.f, 8.f, 0.f));

	ModeButtons.Add(AddButton(*Box, TEXT("Raw"), static_cast<int32>(ECastleWallViewMode::RawOutlines)));
	ModeButtons.Add(AddButton(*Box, TEXT("Floor plan"), static_cast<int32>(ECastleWallViewMode::FloorPlan)));
	ModeButtons.Add(AddButton(*Box, TEXT("Walls 3D"), static_cast<int32>(ECastleWallViewMode::Walls3D)));

	if (UCastleWallSubsystem* Walls = GetWalls())
	{
		ViewModeHandle = Walls->OnViewModeChanged.AddUObject(this, &UCastleViewModeWidget::Highlight);
		Highlight(Walls->GetViewMode());
	}
}

void UCastleViewModeWidget::NativeDestruct()
{
	if (UCastleWallSubsystem* Walls = GetWalls())
	{
		Walls->OnViewModeChanged.Remove(ViewModeHandle);
	}

	Super::NativeDestruct();
}

UCastleToolbarButton* UCastleViewModeWidget::AddButton(UHorizontalBox& Box, const FString& Label, int32 Index)
{
	UCastleToolbarButton* Button = WidgetTree->ConstructWidget<UCastleToolbarButton>(UCastleToolbarButton::StaticClass());
	Button->Setup(Index, UCastleToolbarButton::FOnChosen::CreateUObject(this, &UCastleViewModeWidget::Choose));
	Button->SetSelected(false);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(FText::FromString(Label));
	Text->SetFontSize(12.f);
	Button->SetContent(Text);

	UHorizontalBoxSlot* ButtonSlot = Box.AddChildToHorizontalBox(Button);
	ButtonSlot->SetPadding(FMargin(2.f, 0.f));
	return Button;
}

void UCastleViewModeWidget::Highlight(ECastleWallViewMode ViewMode)
{
	for (int32 Index = 0; Index < ModeButtons.Num(); ++Index)
	{
		ModeButtons[Index]->SetSelected(Index == static_cast<int32>(ViewMode));
	}
}

void UCastleViewModeWidget::Choose(int32 Index)
{
	UCastleWallSubsystem* Walls = GetWalls();
	if (Walls && ModeButtons.IsValidIndex(Index))
	{
		Walls->SetViewMode(static_cast<ECastleWallViewMode>(Index));
	}
}

UCastleWallSubsystem* UCastleViewModeWidget::GetWalls() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UCastleWallSubsystem>() : nullptr;
}
