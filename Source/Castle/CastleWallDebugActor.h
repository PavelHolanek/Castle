#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CastleWallDebugActor.generated.h"

class UBillboardComponent;
class ULineBatchComponent;

USTRUCT()
struct FCastleDebugWall
{
	GENERATED_BODY()

	/** Index into the actor's Nodes. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "0"))
	int32 NodeA = 0;

	/** Index into the actor's Nodes. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "0"))
	int32 NodeB = 1;

	/** Width to the left of the reference line, looking from NodeA to NodeB from above. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "0", Units = "cm"))
	float WidthLeft = 100.f;

	/** Width to the right of the reference line, looking from NodeA to NodeB from above. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "0", Units = "cm"))
	float WidthRight = 100.f;

	/**
	 * 0 = straight. Otherwise tan(angle / 4) of the arc's included angle, 1 = half circle.
	 * Positive runs counterclockwise seen from above, so the arc bows out to the right of NodeA->NodeB.
	 */
	UPROPERTY(EditAnywhere, Category = "Wall")
	float Bulge = 0.f;
};

/**
 * Debug view of the wall model. Draws each wall's reference line (black), both sides (red) and the
 * end caps (blue) as they are before junctions get resolved. Select the actor to drag its nodes in the
 * level viewport; the lines redraw on every change.
 */
UCLASS()
class CASTLE_API ACastleWallDebugActor : public AActor
{
	GENERATED_BODY()

public:
	ACastleWallDebugActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;

protected:
	/** Node positions relative to the actor. Z is ignored. */
	UPROPERTY(EditAnywhere, Category = "Walls", meta = (MakeEditWidget))
	TArray<FVector> Nodes;

	UPROPERTY(EditAnywhere, Category = "Walls")
	TArray<FCastleDebugWall> Walls;

	/** Maximum angle between two neighbouring points of a drawn arc. */
	UPROPERTY(EditAnywhere, Category = "Walls|Drawing", meta = (ClampMin = "0.5", ClampMax = "45", Units = "Degrees"))
	float ArcStep = 5.f;

	UPROPERTY(EditAnywhere, Category = "Walls|Drawing", meta = (ClampMin = "0"))
	float LineThickness = 3.f;

	UPROPERTY(EditAnywhere, Category = "Walls|Drawing", meta = (ClampMin = "0"))
	float NodeSize = 12.f;

	/** Height above the actor at which the lines are drawn. */
	UPROPERTY(EditAnywhere, Category = "Walls|Drawing", meta = (Units = "cm"))
	float DrawHeight = 5.f;

private:
	void Redraw();

	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<ULineBatchComponent> Lines;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
