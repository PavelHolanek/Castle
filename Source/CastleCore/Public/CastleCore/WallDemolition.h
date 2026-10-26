#pragma once

#include "CastleCore/CastleCoreApi.h"
#include "CastleCore/WallLimits.h"
#include "CastleCore/WallNetwork.h"

namespace Castle::Core
{
	/**
	 * Removes the wall the way the player does, so that no leftovers stay behind: a node left without walls goes too,
	 * and two walls left continuing each other at a node join into one again (see WallNetwork::MergeWalls), as after
	 * removing the stem of a T. They stay apart when the joined wall would break Limits, e.g. be too long.
	 * False when the wall doesn't exist.
	 */
	CASTLECORE_API bool DemolishWall(WallNetwork& Network, WallId Id, const WallLimits& Limits);
}
