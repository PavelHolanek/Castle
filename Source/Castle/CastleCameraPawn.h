#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CastleCameraPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/**
 * Strategy camera. The pawn sits on the ground as a pivot and the camera looks at it
 * from the end of a spring arm.
 *   WASD        - pan along the ground, relative to the current camera yaw
 *   Q / E       - rotate around the pivot
 *   Mouse wheel - zoom (spring arm length)
 */
UCLASS()
class CASTLE_API ACastleCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ACastleCameraPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Pan speed in cm/s at maximum zoom. Scales down proportionally when zoomed in. */
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (ClampMin = "0"))
	float MoveSpeed = 4000.f;

	/** Rotation speed in degrees per second. */
	UPROPERTY(EditAnywhere, Category = "Camera|Rotation", meta = (ClampMin = "0"))
	float RotationSpeed = 90.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0"))
	float MinZoom = 500.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1"))
	float MaxZoom = 4000.f;

	/** Spring arm length change per mouse wheel notch. */
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0"))
	float ZoomStep = 250.f;

	/** How fast the arm length catches up with the target zoom. 0 = instant. */
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0"))
	float ZoomInterpSpeed = 8.f;

private:
	void CreateInputMappings();

	void Move(const FInputActionValue& Value);
	void Rotate(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RotateAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	float TargetZoom = 0.f;
};
