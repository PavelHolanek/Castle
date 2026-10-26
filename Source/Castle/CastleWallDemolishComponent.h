#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallLimits.h"
#include "CastleCore/WallNetwork.h"
#include "CastleToolComponent.h"
#include "CastleWallDemolishComponent.generated.h"

class UInputAction;

/**
 * The demolish tool of a player controller:
 *   Left click  - removes the wall under the cursor
 *   Right click - back to the build tool
 * The wall under the cursor (within the controller's pick radius for thin walls) shows highlighted. Nodes left without
 * walls go with it, and two walls left continuing each other join into one again (see Castle::Core::DemolishWall).
 */
UCLASS()
class CASTLE_API UCastleWallDemolishComponent : public UCastleToolComponent
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void CreateMappings(UInputMappingContext& Context) override;
	virtual void BindInput(UEnhancedInputComponent& Input) override;
	virtual void OnDeactivated() override;

private:
	/** The wall the cursor points at. Invalid when there is none. */
	Castle::Core::WallId FindTarget() const;

	void Demolish();
	void Leave();
	void ShowInfo() const;

	/** Walls left at a node join only when the result keeps within these. */
	Castle::Core::WallLimits Limits;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DemolishAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LeaveAction;
};
