#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "CastleBuildToolbarWidget.generated.h"

class ACastlePlayerController;
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
 * Bottom toolbar of the building tools: a button per wall type, which picks it for the build tool, and one for the
 * demolish tool. The button of what the active tool does is highlighted. Built in code, so it needs no widget blueprint.
 */
UCLASS()
class CASTLE_API UCastleBuildToolbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the toolbar with the controller's tools and keeps it in step with them. */
	void SetController(ACastlePlayerController* InController);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	/** Stops following the controller's tools. */
	void Unbind();

	/** Highlights the buttons according to the active tool and the selected wall type. */
	void Refresh();

	void ChooseWallType(int32 Index);
	void ChooseDemolish(int32 Index);

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ButtonBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCastleToolbarButton>> TypeButtons;

	UPROPERTY(Transient)
	TObjectPtr<UCastleToolbarButton> DemolishButton;

	TWeakObjectPtr<ACastlePlayerController> Controller;
	FDelegateHandle WallTypeHandle;
	FDelegateHandle ToolHandle;
};
