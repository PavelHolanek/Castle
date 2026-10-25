#include "CastleCore/Polygon.h"

#include <utility>

namespace Castle::Core
{
	double GetSignedArea(const std::vector<Vec2>& Polygon)
	{
		double DoubleArea = 0.0;
		for (size_t Index = 0; Index < Polygon.size(); ++Index)
		{
			DoubleArea += Cross(Polygon[Index], Polygon[(Index + 1) % Polygon.size()]);
		}
		return 0.5 * DoubleArea;
	}

	std::vector<Vec2> MakeRegularPolygon(Vec2 Center, double Radius, int Segments)
	{
		std::vector<Vec2> Polygon;
		Polygon.reserve(static_cast<size_t>(Segments));
		for (int Index = 0; Index < Segments; ++Index)
		{
			Polygon.push_back(PointOnCircle(Center, Radius, 2.0 * Pi * Index / Segments));
		}
		return Polygon;
	}

	std::vector<Vec2> ClipToConvex(const std::vector<Vec2>& Subject, const std::vector<Vec2>& Clip)
	{
		// Cut away what lies to the right of each of Clip's edges in turn.
		std::vector<Vec2> Output = Subject;
		for (size_t ClipIndex = 0; ClipIndex < Clip.size() && !Output.empty(); ++ClipIndex)
		{
			const Vec2 EdgeStart = Clip[ClipIndex];
			const Vec2 Edge = Clip[(ClipIndex + 1) % Clip.size()] - EdgeStart;

			const std::vector<Vec2> Input = std::move(Output);
			Output.clear();
			for (size_t Index = 0; Index < Input.size(); ++Index)
			{
				const Vec2 Current = Input[Index];
				const Vec2 Next = Input[(Index + 1) % Input.size()];
				const double CurrentSide = Cross(Edge, Current - EdgeStart);
				const double NextSide = Cross(Edge, Next - EdgeStart);

				if (CurrentSide >= 0.0)
				{
					Output.push_back(Current);
				}
				if ((CurrentSide >= 0.0) != (NextSide >= 0.0))
				{
					Output.push_back(Current + (Next - Current) * (CurrentSide / (CurrentSide - NextSide)));
				}
			}
		}
		return Output;
	}
}
