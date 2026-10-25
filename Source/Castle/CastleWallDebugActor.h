#pragma once

#include "CoreMinimal.h"
#include "CastleWallMesh.h"
#include "CastleWallViewMode.h"
#include "GameFramework/Actor.h"
#include "CastleWallDebugActor.generated.h"

class UBillboardComponent;
class UDynamicMeshComponent;
class ULineBatchComponent;
class UMaterialInterface;

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
 * Debug view of the wall model. Draws the floor plan: each wall's reference line (black), its body between the cuts
 * (red sides, blue cuts) and the node pieces (orange), listed in FloorPlanPieces. ViewMode switches to the raw
 * outlines with the junction paths instead. Select the actor to drag its nodes in the level viewport; the lines
 * redraw on every change.
 *
 * Walls and nodes outside the default WallLimits are drawn in magenta and listed in LimitViolations; such nodes are
 * left unresolved. The junction paths are listed in JunctionCorners.
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

	/** Longest a side extension at a junction may get, as a multiple of the wider wall's width on that side. */
	UPROPERTY(EditAnywhere, Category = "Walls|Junctions", meta = (ClampMin = "0"))
	float MaxExtensionFactor = 2.f;

	/** Nodes (by index) whose outer corner, the gap over 180 degrees, is beveled instead of sharp. */
	UPROPERTY(EditAnywhere, Category = "Walls|Junctions")
	TArray<int32> BeveledNodes;

	/** Height above the actor at which the lines are drawn. */
	UPROPERTY(EditAnywhere, Category = "Walls|Drawing", meta = (Units = "cm"))
	float DrawHeight = 5.f;

	UPROPERTY(EditAnywhere, Category = "Walls|Drawing")
	ECastleWallViewMode ViewMode = ECastleWallViewMode::FloorPlan;

	/** How the walls look in the Walls 3D view. */
	UPROPERTY(EditAnywhere, Category = "Walls|Drawing")
	FCastleWallStyle Style;

	/** Limits broken by the walls and nodes above, by index. Refreshed on every redraw. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Walls")
	TArray<FString> LimitViolations;

	/**
	 * How the outline gets from one wall's side to the next, going counterclockwise around each node
	 * (seen from above). Positions are relative to the actor, like Nodes. Refreshed on every redraw.
	 */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Walls")
	TArray<FString> JunctionCorners;

	/**
	 * Where each wall is cut off at its nodes (distance along the reference line) and the outline of each node piece.
	 * Positions are relative to the actor. Refreshed on every redraw.
	 */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Walls")
	TArray<FString> FloorPlanPieces;

private:
	void Redraw();

	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<ULineBatchComponent> Lines;

	UPROPERTY(VisibleAnywhere, Category = "Walls")
	TObjectPtr<UDynamicMeshComponent> WallMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
