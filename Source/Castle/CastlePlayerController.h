#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CastlePlayerController.generated.h"

class UCastleBuildToolbarWidget;
class UCastleToolComponent;
class UCastleViewModeWidget;
class UCastleWallBuildComponent;
class UCastleWallDemolishComponent;
class UInputAction;
class UInputMappingContext;

/**
 * Player controller for the strategy view: the mouse cursor stays visible and is tracked on the ground, where the
 * active tool uses it. The tools switch with the toolbar or keys, whichever tool is active:
 *   1, 2, 3...  - picks the wall type and the build tool
 *   X           - switches between the demolish and the build tool
 */
UCLASS()
class CASTLE_API ACastlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACastlePlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	UCastleWallBuildComponent* GetWallBuild() const { return WallBuild; }
	UCastleWallDemolishComponent* GetWallDemolish() const { return WallDemolish; }

	/** The tool the mouse works with. */
	UCastleToolComponent* GetActiveTool() const { return ActiveTool; }

	/** Makes Tool the one the mouse works with; the previous one drops whatever it was in the middle of. */
	void SetActiveTool(UCastleToolComponent* Tool);

	/** Picks the wall type to build, which switches to the build tool too. */
	void SelectWallType(int32 Index);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveToolChanged, UCastleToolComponent* /* Tool */);

	/** Fires whenever another tool becomes active. */
	FOnActiveToolChanged OnActiveToolChanged;

	/**
	 * Where the mouse cursor points at the ground plane, and how far PickRadiusPixels reach on the ground there.
	 * Always the ground, even over walls: from any angle that is what the player aims at.
	 * False when the cursor is outside the viewport or doesn't point at the ground.
	 */
	bool GetCursorOnGround(FVector& OutPoint, double& OutPickRadius) const;

	/** Console: adds a straight wall between two points (world X and Y in cm). Ends within 1 cm of a node join it. */
	UFUNCTION(Exec)
	void CastleAddWall(float StartX, float StartY, float EndX, float EndY, float Width);

	/** Console: removes all walls. */
	UFUNCTION(Exec)
	void CastleClearWalls();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
	TObjectPtr<UCastleWallBuildComponent> WallBuild;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
	TObjectPtr<UCastleWallDemolishComponent> WallDemolish;

	UPROPERTY(Transient)
	TObjectPtr<UCastleBuildToolbarWidget> Toolbar;

	/** Development switch of how the walls are shown. Not created in shipping builds. */
	UPROPERTY(Transient)
	TObjectPtr<UCastleViewModeWidget> ViewModeSwitch;

	/** Height of the ground plane the cursor is projected onto. */
	UPROPERTY(EditAnywhere, Category = "Cursor", meta = (Units = "cm"))
	float GroundHeight = 0.f;

	/** How close to the cursor, in screen pixels, something has to be to get picked. */
	UPROPERTY(EditAnywhere, Category = "Cursor", meta = (ClampMin = "1"))
	float PickRadiusPixels = 15.f;

	/** Shows the cursor's ground position on screen. */
	UPROPERTY(EditAnywhere, Category = "Cursor")
	bool bShowCursorInfo = true;

private:
	void CreateToolMappings();
	void ToggleDemolish();

	UPROPERTY(Transient)
	TObjectPtr<UCastleToolComponent> ActiveTool;

	/** Switching tools, in effect whichever tool is active. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ToolMappingContext;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> SelectWallTypeActions;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleDemolishAction;
};
