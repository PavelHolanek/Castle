#pragma once

#include "CoreMinimal.h"
#include "CastleCore/WallNetwork.h"
#include "CastleWallDrawing.h"
#include "CastleWallMesh.generated.h"

class UDynamicMeshComponent;
class UMaterialInterface;

/** How built walls look. A single debug style for now; wall types will each bring their own. */
USTRUCT()
struct FCastleWallStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "1", Units = "cm"))
	float Height = 600.f;

	UPROPERTY(EditAnywhere, Category = "Wall")
	FLinearColor Color = FLinearColor(0.42f, 0.38f, 0.32f);
};

namespace CastleWallMesh
{
	/** The engine material the walls are shown with; its "Color" parameter takes the style's color. */
	const TCHAR* const BaseMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	/** Color of what is outside the limits: wall bodies, and the joints of nodes. */
	const FLinearColor FaultColor(0.75f, 0.04f, 0.04f);

	/**
	 * How far the joint of a node outside the limits may reach from the node, as a multiple of the total width of the
	 * widest wall there. Just below the smallest joint angle the joint is about that large, so it doesn't jump when
	 * it turns faulty; sharper angles get their tips cut off instead of reaching far out.
	 */
	constexpr double FaultJointReach = 2.0;

	/** How much higher a faulty joint stands than the walls, so that it covers them instead of flickering with them. */
	constexpr double FaultJointRise = 5.0;

	/**
	 * Shows Network in Mesh as solid walls: every wall body and node piece of its floor plan extruded from Z = 0 to the
	 * style's height, with a flat normal per face and arcs split as Options say. Positions are relative to Mesh, as the
	 * network's are to the drawing transform. A null Network empties and hides Mesh.
	 *
	 * What breaks Options' limits shows in FaultColor. Walls meeting at a node outside the limits end in plain caps
	 * there, covered by the node's joint cut down to FaultJointReach.
	 */
	void ShowWalls(UDynamicMeshComponent& Mesh, const Castle::Core::WallNetwork* Network, const FCastleWallDrawOptions& Options,
		const FCastleWallStyle& Style, UMaterialInterface* BaseMaterial);
}
