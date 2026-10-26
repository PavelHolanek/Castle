#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CastleToolComponent.generated.h"

class UCastleWallSubsystem;
class UEnhancedInputComponent;
class UInputMappingContext;

/**
 * A tool of ACastlePlayerController, which keeps exactly one of them active (see SetActiveTool). A tool's controls are
 * in effect, and it ticks, only while it is active: the controller calls Activate and Deactivate, which UActorComponent
 * already uses to switch the tick on and off.
 */
UCLASS(Abstract)
class CASTLE_API UCastleToolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCastleToolComponent();

	/** Creates the tool's controls and binds them to Input. The owning player controller calls it. */
	void SetupInput(UEnhancedInputComponent& Input);

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

protected:
	/** Creates the tool's actions and maps their keys in Context, which is in effect while the tool is active. */
	virtual void CreateMappings(UInputMappingContext& Context) PURE_VIRTUAL(UCastleToolComponent::CreateMappings, );

	virtual void BindInput(UEnhancedInputComponent& Input) PURE_VIRTUAL(UCastleToolComponent::BindInput, );

	/** The tool stopped being active, time to drop whatever it was in the middle of. */
	virtual void OnDeactivated() {}

	/** See ACastlePlayerController::GetCursorOnGround. False also without such a controller. */
	bool GetCursorOnGround(FVector& OutPoint, double& OutPickRadius) const;

	UCastleWallSubsystem* GetWalls() const;

private:
	/** Puts the mapping context in effect while the tool is active, and out of it otherwise. */
	void ApplyMappingContext() const;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;
};
