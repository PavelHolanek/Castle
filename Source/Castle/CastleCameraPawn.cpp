#include "CastleCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

ACastleCameraPawn::ACastleCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));
	SpringArm->TargetArmLength = 2000.f;
	// The camera floats freely above the map; don't pull it in when something gets in between.
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ACastleCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	TargetZoom = FMath::Clamp(SpringArm->TargetArmLength, MinZoom, MaxZoom);
}

void ACastleCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaSeconds, ZoomInterpSpeed);
}

void ACastleCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!MappingContext)
	{
		CreateInputMappings();
	}

	if (const APlayerController* PlayerController = GetController<APlayerController>())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACastleCameraPawn::Move);
	EnhancedInput->BindAction(RotateAction, ETriggerEvent::Triggered, this, &ACastleCameraPawn::Rotate);
	EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ACastleCameraPawn::Zoom);
}

void ACastleCameraPawn::CreateInputMappings()
{
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_CameraMove"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	RotateAction = NewObject<UInputAction>(this, TEXT("IA_CameraRotate"));
	RotateAction->ValueType = EInputActionValueType::Axis1D;

	ZoomAction = NewObject<UInputAction>(this, TEXT("IA_CameraZoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Camera"));

	// Move value: X = right, Y = forward. A key press yields +1 on X;
	// swizzle moves it to Y, negate flips the direction.
	auto MapMoveKey = [this](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};
	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);

	MappingContext->MapKey(RotateAction, EKeys::E);
	MappingContext->MapKey(RotateAction, EKeys::Q).Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));

	MappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
}

void ACastleCameraPawn::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	const FVector Direction = GetActorForwardVector() * Input.Y + GetActorRightVector() * Input.X;

	// Pan faster when zoomed out so the on-screen speed stays roughly the same.
	const float Speed = MoveSpeed * SpringArm->TargetArmLength / MaxZoom;

	AddActorWorldOffset(Direction.GetClampedToMaxSize(1.f) * Speed * GetWorld()->GetDeltaSeconds());
}

void ACastleCameraPawn::Rotate(const FInputActionValue& Value)
{
	const float YawDelta = Value.Get<float>() * RotationSpeed * GetWorld()->GetDeltaSeconds();
	AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
}

void ACastleCameraPawn::Zoom(const FInputActionValue& Value)
{
	// Wheel up (positive) zooms in.
	TargetZoom = FMath::Clamp(TargetZoom - Value.Get<float>() * ZoomStep, MinZoom, MaxZoom);
}
