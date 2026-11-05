#pragma once

#include "lib/lib_math.h"

typedef struct {
    uint32_t *buffer;
    int width;
    int height;
    int half_width;
    int half_height;
} Screen;

typedef struct {
    Vec3 vertex;
    Vec3 normal;
    Vec2 uv;
    int attributes;
} Ver;

typedef struct {
    Ver vertices[3];
    Vec3 normal;
    int attributes;
} Tri;

typedef struct {
    int id;
    Tri *triangles_local;
    Tri *triangles_transformed;
    unsigned int num_triangles;
    double rotation; // around Z axis, rad
    double radius;
    int attributes;
} Mesh;

Mesh* renderer3d_load_mesh(char* filename);

void renderer3d_render_mesh(Mesh *mesh, Screen *screen);
