#include "CastleCore/Polygon.h"

#include <cmath>
#include <cstddef>
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

	void SimplifyPolygon(std::vector<Vec2>& Polygon)
	{
		bool bChanged = true;
		while (bChanged && Polygon.size() >= 3)
		{
			bChanged = false;
			const size_t Count = Polygon.size();
			for (size_t Index = 0; Index < Count; ++Index)
			{
				const Vec2 Previous = Polygon[(Index + Count - 1) % Count];
				const Vec2 Current = Polygon[Index];
				const Vec2 Next = Polygon[(Index + 1) % Count];

				const double Span = Distance(Previous, Next);
				const bool bRedundant = IsNearlyEqual(Current, Next)
					|| Span <= DistanceTolerance
					|| std::abs(Cross(Next - Previous, Current - Previous)) / Span <= DistanceTolerance;
				if (bRedundant)
				{
					Polygon.erase(Polygon.begin() + static_cast<std::ptrdiff_t>(Index));
					bChanged = true;
					break;
				}
			}
		}
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
