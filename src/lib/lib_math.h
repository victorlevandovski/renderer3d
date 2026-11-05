#pragma once

#include <stdbool.h>

#define EPSILON 0.000001
#define RAD2DEG 57.295779513082323
#define DEG2RAD 0.017453292519943295

#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

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

extern Vec2 VEC2_ZERO;

Vec2 vec2_add(Vec2 v1, Vec2 v2);
Vec2 vec2_subtract(Vec2 v1, Vec2 v2);
Vec2 vec2_multiply(Vec2 v, double multiplier);
Vec2 vec2_resize(Vec2 v, double length_diff);
Vec2 vec2_normalize(Vec2 v);
Vec2 vec2_rotate(Vec2 v, double angle);
double vec2_length(Vec2 v);
double vec2_distance(Vec2 v1, Vec2 v2);
double vec2_angle(Vec2 v);
double vec2_acute_angle(Vec2 v1, Vec2 v2);

double remap(double source_from, double source_to, double destination_from, double destination_to, double value);
float remapf(float source_from, float source_to, float destination_from, float destination_to, float value);

double rand_between(double min, double max);

double normalize_angle(double a);
int rad_to_compass_deg(double a);
