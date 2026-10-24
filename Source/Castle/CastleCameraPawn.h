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
 * Runs on real time, so game speed doesn't change how the camera moves.
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

	/** Pan speed in cm/s when fully zoomed in. In between, the speed follows the spring arm length linearly. */
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (ClampMin = "0"))
	float MinZoomMoveSpeed = 5500.f;

	/** Pan speed in cm/s when fully zoomed out. */
	UPROPERTY(EditAnywhere, Category = "Camera|Movement", meta = (ClampMin = "0"))
	float MaxZoomMoveSpeed = 6000.f;

	/** Rotation speed in degrees per second. */
	UPROPERTY(EditAnywhere, Category = "Camera|Rotation", meta = (ClampMin = "0"))
	float RotationSpeed = 90.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0"))
	float MinZoom = 500.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1"))
	float MaxZoom = 4000.f;

	/** Spring arm length is divided (zoom in) or multiplied (zoom out) by this per mouse wheel notch. */
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1"))
	float ZoomStepFactor = 1.15f;

	/** How fast the arm length catches up with the target zoom. 0 = instant. */
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0"))
	float ZoomInterpSpeed = 8.f;

private:
	void CreateInputMappings();

	void Move(const FInputActionValue& Value);
	void Rotate(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);

	/** Frame time without time dilation. */
	float GetRealDeltaSeconds() const;

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
