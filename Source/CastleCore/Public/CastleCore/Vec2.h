#pragma once

#include <cmath>
#include <numbers>

/**
 * Plain C++ core of the wall system. Nothing in this namespace depends on Unreal.
 *
 * The 2D plane is right-handed: the Y axis is 90 degrees counterclockwise from the X axis, so "left"
 * and "counterclockwise" mean what they do on paper. Unreal's XY plane seen from above is mirrored,
 * so the game maps Unreal (X, Y) to core (X, -Y). Units are centimeters, as in Unreal.
 */
namespace Castle::Core
{
	inline constexpr double Pi = std::numbers::pi;

	/** Distances below this are treated as zero. */
	inline constexpr double DistanceTolerance = 1e-6;

	/** Angles (radians) and dimensionless values below this are treated as zero. */
	inline constexpr double AngleTolerance = 1e-9;

	struct Vec2
	{
		double X = 0.0;
		double Y = 0.0;

		constexpr Vec2 operator+(Vec2 Other) const { return {X + Other.X, Y + Other.Y}; }
		constexpr Vec2 operator-(Vec2 Other) const { return {X - Other.X, Y - Other.Y}; }
		constexpr Vec2 operator-() const { return {-X, -Y}; }
		constexpr Vec2 operator*(double Scale) const { return {X * Scale, Y * Scale}; }
		constexpr Vec2 operator/(double Scale) const { return {X / Scale, Y / Scale}; }

		constexpr Vec2& operator+=(Vec2 Other) { X += Other.X; Y += Other.Y; return *this; }
		constexpr Vec2& operator-=(Vec2 Other) { X -= Other.X; Y -= Other.Y; return *this; }

		constexpr bool operator==(const Vec2&) const = default;
	};

	constexpr Vec2 operator*(double Scale, Vec2 V) { return V * Scale; }

	constexpr double Dot(Vec2 A, Vec2 B) { return A.X * B.X + A.Y * B.Y; }

	/** Z of the 3D cross product: positive when B points counterclockwise from A. */
	constexpr double Cross(Vec2 A, Vec2 B) { return A.X * B.Y - A.Y * B.X; }

	/** V rotated 90 degrees counterclockwise, i.e. pointing to the left of V. */
	constexpr Vec2 PerpLeft(Vec2 V) { return {-V.Y, V.X}; }

	inline double Length(Vec2 V) { return std::sqrt(Dot(V, V)); }

	inline double Distance(Vec2 A, Vec2 B) { return Length(B - A); }

	/** Unit vector in the direction of V, or the zero vector when V is (nearly) zero. */
	inline Vec2 Normalized(Vec2 V)
	{
		const double Len = Length(V);
		return Len > DistanceTolerance ? V / Len : Vec2{};
	}

	/** Point at the given angle (radians, counterclockwise from +X) and distance from Center. */
	inline Vec2 PointOnCircle(Vec2 Center, double Radius, double Angle)
	{
		return {Center.X + Radius * std::cos(Angle), Center.Y + Radius * std::sin(Angle)};
	}

	inline bool IsNearlyEqual(Vec2 A, Vec2 B, double Tolerance = DistanceTolerance)
	{
		return Distance(A, B) <= Tolerance;
	}
}
