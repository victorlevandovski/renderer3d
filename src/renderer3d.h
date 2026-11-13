#pragma once

#include "lib/lib_math.h"
#include "renderer3d_shader.h"

#define MESH_LIST_MAX_MESHES 1024
#define LIGHT_LIST_MAX_LIGHTS 4
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
    Vec3 vertex; // z used as w after vertex is projected on screen
    Vec3 normal;
    Vec2 uv;
    double w_inv;
    double light;
    int attributes;
} Vertex;

typedef struct {
    Vertex vertices[3];
    Vec3 normal;
    double w_inv_max;
    double area;
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
    Transform world_transform;
    Transform camera_transform;
    double radius;
    int state;
    int attributes;
} Mesh;

typedef struct {
    Mesh *meshes[MESH_LIST_MAX_MESHES];
    unsigned int num_meshes;
} MeshList;

typedef struct {
    Transform world_transform;
    Transform camera_transform;
    Vec3 normal;
    double brightness;
    int state;
} Light;

typedef struct {
    Light *lights[LIGHT_LIST_MAX_LIGHTS];
    unsigned int num_lights;
} LightList;

typedef struct {
    Triangle *triangles[RENDER_LIST_MAX_TRIANGLES];
    unsigned int num_triangles;
} RenderList;

typedef struct {
    Transform transform;
    double fov;
    double near_plane;
} Camera;

typedef struct {
    uint8_t r, g, b;
} Col3i;

extern Col3i COL3I_BLACK;
extern Col3i COL3I_WHITE;

static double *z_buffer;
static Texture *renderer_texture;

#define pixel(s, x, y) ((s)->buffer[(((s)->height - (y) - 1) * (s)->width) + (x)])
#define col3i_to_uint32_t(c) (((uint32_t) (c).r << 16) | ((uint32_t) (c).g << 8) | (c).b)

Mesh* renderer3d_load_mesh(char* filename);

void renderer3d_init(const Screen *screen, Texture *texture);
bool renderer3d_is_transformed_mesh_in_viewport(const Mesh* mesh, const Camera *camera);
void renderer3d_compute_camera_transform(const Transform *world_transform, Transform *camera_transform, const Camera *camera);
void renderer3d_transform_mesh_triangles(const Mesh *mesh, const Camera *camera);
int renderer3d_compute_render_list(RenderList *render_list, MeshList *mesh_list, Camera *camera);
void renderer3d_compute_triangle_lighting(Triangle *triangle, const LightList *light_list);
void renderer3d_compute_triangle_for_rasterization(Triangle *triangle, Camera *camera, Screen *screen);
void renderer3d_render_triangle(Triangle *triangle, Screen *screen);
void renderer3d_render(const RenderList *render_list, LightList *light_list, Camera *camera, Screen *screen);
