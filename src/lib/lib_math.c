#include "lib_math.h"

#include <stdlib.h>
#include <math.h>

Vec2 VEC2_ZERO = {0.0, 0.0};
Vec3 VEC3_ZERO = {0.0, 0.0, 0.0};

int sign(double value)
{
    return value >= 0 ? 1 : -1;
}

bool equals_approx(double value1, double value2)
{
    return fabs(value2 - value1) < EPSILON;
}

double vec2_length(Vec2 v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

double vec2_distance(Vec2 a, Vec2 b)
{
    return sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0));
}

Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return (Vec2) {a.x + b.x, a.y + b.y};
}

Vec2 vec2_subtract(Vec2 a, Vec2 b)
{
    return (Vec2) {a.x - b.x, a.y - b.y};
}

Vec2 vec2_multiply(Vec2 v, double multiplier)
{
    return (Vec2) {v.x * multiplier, v.y * multiplier};
}

Vec2 vec2_resize(Vec2 v, double length_diff)
{
    return vec2_multiply(v, 1 + length_diff / vec2_length(v));
}

Vec2 vec2_normalize(Vec2 v)
{
    double length = vec2_length(v);
    return (Vec2) {v.x / length, v.y / length};
}

Vec2 vec2_rotate(Vec2 v, double angle)
{
    double s = sin(angle);
    double c = cos(angle);
    return (Vec2) {v.x * c - v.y * s, v.x * s + v.y * c};
}

double vec2_angle(Vec2 v)
{
    double a = atan2(v.y, v.x);

    if (a < 0)
        a += M_PI * 2.0;

    return a;
}

double vec2_acute_angle(Vec2 a, Vec2 b)
{
    double len_product = vec2_length(a) * vec2_length(b);

    if (len_product == 0.0)
        return 0.0;

    double vec_cross_magnitude = 0.3; // test test

    double sin_theta = fabs(vec_cross_magnitude) / len_product;
    sin_theta = fmin(fmax(sin_theta, 0.0), 1.0);

    return asin(sin_theta);
}

Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return (Vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 vec3_subtract(Vec3 a, Vec3 b)
{
    return (Vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 vec3_normalize(Vec3 v)
{
    double length = vec3_length(v);
    return (Vec3) {v.x / length, v.y / length, v.z / length};
}

Vec3 vec3_rotate_xy(Vec3 v, double angle)
{
    double s = sin(angle);
    double c = cos(angle);
    return (Vec3) { v.x * c - v.y * s, v.x * s + v.y * c, v.z };
}

Vec3 vec3_average(Vec3 a, Vec3 b, Vec3 c)
{
    return (Vec3) {
        .x = (a.x + b.x + c.x) / 3.0,
        .y = (a.y + b.y + c.y) / 3.0,
        .z = (a.z + b.z + c.z) / 3.0
    };
}

double vec3_length(Vec3 v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

double vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double vec3_angle(Vec3 a, Vec3 b)
{
    return acos(vec3_dot(a, b) / (vec3_length(a) * vec3_length(b)));
}

double vec3_angle_xy(Vec3 v)
{
    return atan2(v.y, v.x);
}

double remap(double source_from, double source_to, double destination_from, double destination_to, double value)
{
    double t = (value - source_from) / (source_to - source_from);

    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    return destination_from + (destination_to - destination_from) * t;
}

float remapf(float source_from, float source_to, float destination_from, float destination_to, float value)
{
    float t = (value - source_from) / (source_to - source_from);

    if (t <= 0.0f) return destination_from;
    if (t >= 1.0f) return destination_to;

    return destination_from + (destination_to - destination_from) * t;
}

double normalize_angle(double a)
{
    return fmod(a, M_PI * 2.0);
}

int rad_to_compass_deg(double a)
{
    int i = (int) round(normalize_angle(M_PI_2 - a) * RAD2DEG);
    if (i < 0) i += 360;
    if (i > 359) i = 0;

    return i;
}

double rand_between(double min, double max)
{
    return min + ((max - min) / RAND_MAX) * rand();
}


bool is_outer_points_line_crossing_circle(const Vec3 a, const Vec3 b, const double radius)
{
    double dx = b.x - a.x;
    double dy = b.z - a.z;
    double l_sq = dx * dx + dy * dy;

    if (l_sq == 0)
        return false;

    double t = (-a.x * dx - a.z * dy) / l_sq;

    if (t <= 0 || t >= 1)
        return false;

    double d_sq = pow(a.x + t*dx, 2) + pow(a.z + t * dy, 2);

    return d_sq <= radius * radius;
}
