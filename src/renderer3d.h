#pragma once

#include "lib/lib_math.h"

#define MESH_LIST_MAX_MESHES 1024
#define RENDER_LIST_MAX_TRIANGLES 32768

#define WIREFRAME_COLOR 0x7744bb

typedef struct {
    uint32_t *buffer;
    int width;
    int height;
    int width_2;
    int height_2;
} Screen;

typedef struct {
    Vec3 vertex;
    Vec3 normal;
    Vec2 uv;
    int attributes;
} Vertex;

typedef struct {
    Vertex vertices[3];
    Vec3 normal;
    int attributes;
} Triangle;

typedef struct {
    Vec3 position;
    double rotation;
    double scale;
} Transform;

extern Transform TRANSFORM_ZERO;

typedef struct {
    int id;
    Triangle *triangles_local;
    Triangle *triangles_transformed;
    unsigned int num_triangles;
    Transform transform;
    Transform transform_camera;
    double radius;
    int state;
    int attributes;
} Mesh;

typedef struct {
    Mesh *meshes[MESH_LIST_MAX_MESHES];
    unsigned int num_meshes;
} MeshList;

typedef struct {
    Triangle *triangles[RENDER_LIST_MAX_TRIANGLES];
    unsigned int num_triangles;
} RenderList;

typedef struct {
    Transform transform;
    double fov;
    double near_plane;
} Camera;

Mesh* renderer3d_load_mesh(char* filename);

bool renderer3d_is_mesh_in_viewport(Mesh* mesh, Camera *camera);
int renderer3d_compute_render_list(RenderList *render_list, MeshList *mesh_list, Camera *camera);
void renderer3d_render(RenderList *render_list, Camera *camera, Screen *screen);
