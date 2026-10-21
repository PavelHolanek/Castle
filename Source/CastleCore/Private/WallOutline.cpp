#include "CastleCore/WallOutline.h"

namespace Castle::Core
{
	namespace
	{
		WallCurve MakeCap(Vec2 Point, Vec2 Tangent, const Wall& Source)
		{
			const Vec2 Left = PerpLeft(Tangent);
			return WallCurve::MakeLine(Point - Left * Source.WidthRight, Point + Left * Source.WidthLeft);
		}
	}

	std::optional<RawWallOutline> BuildRawOutline(const WallNetwork& Network, WallId Id)
	{
		const Wall* Found = Network.FindWall(Id);
		if (!Found)
		{
			return std::nullopt;
		}

		const WallCurve Reference = *Network.GetReferenceCurve(Id);
		return RawWallOutline{
			.Reference = Reference,
			.Left = Reference.Offset(Found->WidthLeft),
			.Right = Reference.Offset(-Found->WidthRight),
			.StartCap = MakeCap(Reference.GetStart(), Reference.TangentAt(0.0), *Found),
			.EndCap = MakeCap(Reference.GetEnd(), Reference.TangentAt(1.0), *Found),
		};
	}
}
