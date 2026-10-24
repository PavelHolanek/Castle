#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CastlePlayerController.generated.h"

class UCastleBuildToolbarWidget;
class UCastleWallBuildComponent;

/**
 * Player controller for the strategy view: the mouse cursor stays visible and is tracked on the ground,
 * where the wall building tool uses it.
 */
UCLASS()
class CASTLE_API ACastlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACastlePlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/**
	 * Where the mouse cursor points at the ground plane, and how far PickRadiusPixels reach on the ground there.
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

	UPROPERTY(Transient)
	TObjectPtr<UCastleBuildToolbarWidget> Toolbar;

	/** Height of the ground plane the cursor is projected onto. */
	UPROPERTY(EditAnywhere, Category = "Cursor", meta = (Units = "cm"))
	float GroundHeight = 0.f;

	/** How close to the cursor, in screen pixels, something has to be to get picked. */
	UPROPERTY(EditAnywhere, Category = "Cursor", meta = (ClampMin = "1"))
	float PickRadiusPixels = 15.f;

	/** Shows the cursor's ground position on screen. */
	UPROPERTY(EditAnywhere, Category = "Cursor")
	bool bShowCursorInfo = true;
};
