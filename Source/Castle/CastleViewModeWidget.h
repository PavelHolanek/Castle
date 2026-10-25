#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CastleWallViewMode.h"
#include "CastleViewModeWidget.generated.h"

class UCastleToolbarButton;
class UCastleWallSubsystem;
class UHorizontalBox;

/**
 * Development switch in the top right corner choosing how the walls are shown: raw outlines with the junction paths,
 * the floor plan, or the walls as the player sees them.
 * The player controller doesn't create it in shipping builds.
 */
UCLASS()
class CASTLE_API UCastleViewModeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UCastleToolbarButton* AddButton(UHorizontalBox& Box, const FString& Label, int32 Index);

	void Highlight(ECastleWallViewMode ViewMode);
	void Choose(int32 Index);

	UCastleWallSubsystem* GetWalls() const;

	/** Indexed by ECastleWallViewMode. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCastleToolbarButton>> ModeButtons;

	FDelegateHandle ViewModeHandle;
};
