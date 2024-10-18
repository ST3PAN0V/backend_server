#pragma once

#include <compare>

namespace geom {

struct Vector2D {
    Vector2D() = default;
    Vector2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Vector2D& operator*=(double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    auto operator<=>(const Vector2D&) const = default;

    double x = 0;
    double y = 0;
};

inline Vector2D operator*(Vector2D lhs, double rhs) {
    return lhs *= rhs;
}

inline Vector2D operator*(double lhs, Vector2D rhs) {
    return rhs *= lhs;
}

struct Point2D {
    Point2D() = default;
    Point2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Point2D& operator+=(const Vector2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    auto operator<=>(const Point2D&) const = default;

    double x = 0;
    double y = 0;
};

inline Point2D operator+(Point2D lhs, const Vector2D& rhs) {
    return lhs += rhs;
}

inline Point2D operator+(const Vector2D& lhs, Point2D rhs) {
    return rhs += lhs;
}

}  // namespace geom
