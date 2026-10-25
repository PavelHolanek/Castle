#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "CastleBuildToolbarWidget.generated.h"

class UCastleWallBuildComponent;
class UHorizontalBox;

/** A toolbar button that tells which one it is when clicked, and never takes keyboard focus away from the game. */
UCLASS()
class CASTLE_API UCastleToolbarButton : public UButton
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_OneParam(FOnChosen, int32 /* Index */);

	UCastleToolbarButton(const FObjectInitializer& ObjectInitializer);

	void Setup(int32 InIndex, FOnChosen InOnChosen);

	/** Highlights the button as the current choice. */
	void SetSelected(bool bSelected);

private:
	UFUNCTION()
	void HandleClicked();

	int32 Index = INDEX_NONE;
	FOnChosen OnChosen;
};

/**
 * Bottom toolbar of the wall building tool: a button per wall type, the selected one highlighted.
 * Built in code, so it needs no widget blueprint.
 */
UCLASS()
class CASTLE_API UCastleBuildToolbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the toolbar with the tool's wall types and keeps it in step with the tool. */
	void SetTool(UCastleWallBuildComponent* InTool);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	void Highlight(int32 SelectedIndex);
	void Choose(int32 Index);

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ButtonBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCastleToolbarButton>> TypeButtons;

	TWeakObjectPtr<UCastleWallBuildComponent> Tool;
	FDelegateHandle SelectionHandle;
};
