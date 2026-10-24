#include "CastleBuildToolbarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CastleWallBuildComponent.h"
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
	if (UCastleWallBuildComponent* CurrentTool = Tool.Get())
	{
		CurrentTool->OnWallTypeSelected.Remove(SelectionHandle);
	}

	Super::NativeDestruct();
}

void UCastleBuildToolbarWidget::SetTool(UCastleWallBuildComponent* InTool)
{
	if (UCastleWallBuildComponent* PreviousTool = Tool.Get())
	{
		PreviousTool->OnWallTypeSelected.Remove(SelectionHandle);
	}
	Tool = InTool;
	ButtonBox->ClearChildren();
	TypeButtons.Reset();
	if (!InTool)
	{
		return;
	}

	const TArray<FCastleWallType>& Types = InTool->GetWallTypes();
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		UCastleToolbarButton* Button = WidgetTree->ConstructWidget<UCastleToolbarButton>(UCastleToolbarButton::StaticClass());
		Button->Setup(Index, UCastleToolbarButton::FOnChosen::CreateUObject(this, &UCastleBuildToolbarWidget::Choose));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(FString::Printf(TEXT("%d  %s\n%.0f cm"), Index + 1, *Types[Index].Name, Types[Index].Width)));
		Label->SetJustification(ETextJustify::Center);
		Label->SetFontSize(14.f);
		Button->SetContent(Label);

		UHorizontalBoxSlot* ButtonSlot = ButtonBox->AddChildToHorizontalBox(Button);
		ButtonSlot->SetPadding(FMargin(4.f, 0.f));
		TypeButtons.Add(Button);
	}

	SelectionHandle = InTool->OnWallTypeSelected.AddUObject(this, &UCastleBuildToolbarWidget::Highlight);
	Highlight(InTool->GetSelectedWallType());
}

void UCastleBuildToolbarWidget::Highlight(int32 SelectedIndex)
{
	for (int32 Index = 0; Index < TypeButtons.Num(); ++Index)
	{
		TypeButtons[Index]->SetBackgroundColor(Index == SelectedIndex ? SelectedButtonColor : IdleButtonColor);
	}
}

void UCastleBuildToolbarWidget::Choose(int32 Index)
{
	if (UCastleWallBuildComponent* CurrentTool = Tool.Get())
	{
		CurrentTool->SelectWallType(Index);
	}
}
