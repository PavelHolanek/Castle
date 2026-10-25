#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallPlacement.h"
#include "Components/ActorComponent.h"
#include "CastleWallBuildComponent.generated.h"

class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;

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
 *   Left click  - picks the start of a wall, then its end, which starts the next wall unless it is an existing node
 *   Right click - drops the wall being placed and ends the chain
 *   1, 2, 3...  - chooses the wall type
 * A click near an existing node (within the controller's pick radius) uses that node. While a wall is being
 * placed, the world shows the network with it, junctions and limits included.
 */
UCLASS()
class CASTLE_API UCastleWallBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCastleWallBuildComponent();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnWallTypeSelected, int32 /* Index */);

	/** Creates the tool's controls and binds them to Input. The owning player controller calls it. */
	void SetupInput(UEnhancedInputComponent& Input);

	const TArray<FCastleWallType>& GetWallTypes() const { return WallTypes; }
	int32 GetSelectedWallType() const { return SelectedWallType; }
	void SelectWallType(int32 Index);

	/** Broadcast whenever a wall type gets selected, by key or otherwise. */
	FOnWallTypeSelected OnWallTypeSelected;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Build")
	TArray<FCastleWallType> WallTypes;

	/**
	 * How far around a node the cursor snaps to it, as a multiple of the total width of the widest wall there.
	 * Never less than the controller's pick radius.
	 */
	UPROPERTY(EditAnywhere, Category = "Build", meta = (ClampMin = "0"))
	float SnapWidthFactor = 0.75f;

private:
	void CreateInputMappings();

	void Place();
	void CancelPlacement();

	/** Shows the wall being placed, or the network as it is when there is none. */
	void UpdatePreview();
	void ShowInfo() const;

	Castle::Core::WallPlacement Placement;
	int32 SelectedWallType = 1;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PlaceAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CancelAction;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> SelectTypeActions;
};
