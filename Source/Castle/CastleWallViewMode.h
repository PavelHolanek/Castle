#pragma once

#include "CoreMinimal.h"
#include "CastleWallViewMode.generated.h"

/** How the walls are shown. */
UENUM()
enum class ECastleWallViewMode : uint8
{
	/** Full-length sides with perpendicular caps, and the junction paths. For checking the junction solver. */
	RawOutlines,

	/** Wall bodies between their cuts, and the node pieces. */
	FloorPlan,

	/** The walls as the player sees them: the floor plan extruded to their height. */
	Walls3D UMETA(DisplayName = "Walls 3D"),
};
