#include "CastleCore/WallDemolition.h"

#include <utility>

namespace Castle::Core
{
	bool DemolishWall(WallNetwork& Network, WallId Id, const WallLimits& Limits)
	{
		const Wall* Found = Network.FindWall(Id);
		if (!Found)
		{
			return false;
		}

		const NodeId Ends[] = {Found->NodeA, Found->NodeB};
		Network.RemoveWall(Id);

		for (const NodeId End : Ends)
		{
			const size_t WallsLeft = Network.GetNodeWalls(End).size();
			if (WallsLeft == 0)
			{
				Network.RemoveNode(End);
			}
			else if (WallsLeft == 2)
			{
				// Joined on a copy first, to see whether the result keeps within the limits.
				WallNetwork Joined = Network;
				const WallId Merged = Joined.MergeWalls(End);
				if (Merged.IsValid() && CheckWall(Joined, Merged, Limits).empty())
				{
					Network = std::move(Joined);
				}
			}
		}
		return true;
	}
}
