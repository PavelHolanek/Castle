#pragma once

#include "CoreMinimal.h"
#include "CastleCore/Vec2.h"

/**
 * Conversions between Unreal and the wall core. The core plane is right-handed while Unreal's XY plane seen
 * from above is mirrored, so Y flips both ways. Both use centimeters.
 */
namespace CastleCoreConversion
{
	inline Castle::Core::Vec2 ToCore(const FVector& Point)
	{
		return {Point.X, -Point.Y};
	}

	inline FVector ToUnreal(Castle::Core::Vec2 Point, double Z)
	{
		return FVector(Point.X, -Point.Y, Z);
	}
}
