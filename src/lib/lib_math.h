#pragma once

#include <stdbool.h>

#define EPSILON 0.000001
#define RAD2DEG 57.295779513082323
#define DEG2RAD 0.017453292519943295

#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define swap(a, b, t) do { (t) = (a); (a) = (b); (b) = (t); } while (0)

int sign(double value);

bool equals_approx(double value1, double value2);

typedef struct {
    double x;
    double y;
} Vec2;

typedef struct {
    float x;
    float y;
} Vec2f;

typedef struct {
    int x;
    int y;
} Vec2i;

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

typedef struct {
    float x;
    float y;
    float z;
} Vec3f;

typedef struct {
    double x;
    double y;
    double z;
    double w;
} Vec4;

extern Vec2 VEC2_ZERO;
extern Vec3 VEC3_ZERO;

Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_subtract(Vec2 a, Vec2 b);
Vec2 vec2_multiply(Vec2 v, double multiplier);
Vec2 vec2_resize(Vec2 v, double length_diff);
Vec2 vec2_normalize(Vec2 v);
Vec2 vec2_rotate(Vec2 v, double angle);
double vec2_length(Vec2 v);
double vec2_distance(Vec2 a, Vec2 b);
double vec2_angle(Vec2 v);
double vec2_acute_angle(Vec2 a, Vec2 b);

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_subtract(Vec3 a, Vec3 b);
Vec3 vec3_normalize(Vec3 v);
Vec3 vec3_rotate_xy(Vec3 v, double angle);
Vec3 vec3_average(Vec3 a, Vec3 b, Vec3 c);
double vec3_length(Vec3 v);
double vec3_dot(Vec3 a, Vec3 b);
double vec3_angle(Vec3 a, Vec3 b);
double vec3_angle_xy(Vec3 v);

double remap(double source_from, double source_to, double destination_from, double destination_to, double value);
float remapf(float source_from, float source_to, float destination_from, float destination_to, float value);

double rand_between(double min, double max);

double normalize_angle(double a);
int rad_to_compass_deg(double a);
bool is_outer_points_line_crossing_circle(Vec3 a, Vec3 b, double radius);
