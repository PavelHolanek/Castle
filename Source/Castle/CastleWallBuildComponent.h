#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallPlacement.h"
#include "CastleToolComponent.h"
#include "CastleWallBuildComponent.generated.h"

class UInputAction;

USTRUCT()
struct FCastleWallType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Wall")
	FString Name;

	/** Total width. The line the player draws runs through the middle. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "0", Units = "cm"))
	float Width = 150.f;
};

/**
 * The wall building tool of a player controller:
 *   Left click  - picks the start of a wall, then its end, which starts the next wall unless it joins an existing
 *                 node or wall
 *   Right click - drops the wall being placed and ends the chain
 * The controller picks the wall type (keys 1, 2, 3...). A click near an existing node (within the controller's pick
 * radius, or more around wide walls) uses that node; a click on a wall splits it there. While a wall is being placed,
 * the world shows the network with it, junctions and limits included.
 */
UCLASS()
class CASTLE_API UCastleWallBuildComponent : public UCastleToolComponent
{
	GENERATED_BODY()

public:
	UCastleWallBuildComponent();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnWallTypeSelected, int32 /* Index */);

	const TArray<FCastleWallType>& GetWallTypes() const { return WallTypes; }
	int32 GetSelectedWallType() const { return SelectedWallType; }
	void SelectWallType(int32 Index);

	/** Broadcast whenever a wall type gets selected, by key or otherwise. */
	FOnWallTypeSelected OnWallTypeSelected;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void CreateMappings(UInputMappingContext& Context) override;
	virtual void BindInput(UEnhancedInputComponent& Input) override;
	virtual void OnDeactivated() override;

	UPROPERTY(EditAnywhere, Category = "Build")
	TArray<FCastleWallType> WallTypes;

	/**
	 * How far around a node the cursor snaps to it, as a multiple of the total width of the widest wall there.
	 * Never less than the controller's pick radius.
	 */
	UPROPERTY(EditAnywhere, Category = "Build", meta = (ClampMin = "0"))
	float SnapWidthFactor = 0.75f;

private:
	void Place();
	void CancelPlacement();

	/** Shows the wall being placed, or the network as it is when there is none. */
	void UpdatePreview();
	void ShowInfo() const;

	Castle::Core::WallPlacement Placement;
	int32 SelectedWallType = 1;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PlaceAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CancelAction;
};
