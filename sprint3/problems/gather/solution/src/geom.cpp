#include "geom.h"

using namespace geom;

Point2D operator*(const Point2D& vec, double num) {
    Point2D res;
    res.x = vec.x * num;
    res.y = vec.y * num;
    return res;
}

Point2D operator+(const Point2D& vec1, const Point2D& vec2) {
    Point2D res;
    res.x = vec1.x + vec2.x;
    res.y = vec1.y + vec2.y;
    return res;
}
