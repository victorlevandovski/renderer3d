#include <stdlib.h>
#include <string.h>

#include "fast_obj.h"

#include "renderer3d.h"

#include <math.h>

Col3i COL3I_BLACK = {
    .r = 0,
    .g = 0,
    .b = 0,
};

Col3i COL3I_WHITE = {
    .r = 255,
    .g = 255,
    .b = 255,
};

Transform TRANSFORM_ZERO = {
    .position = (Vec3) { .x = 0.0, .y = 0.0, .z = 0.0 },
    .rotation = 0.0,
    .scale = 1.0,
};

Mesh* renderer3d_load_mesh(char* filename)
{
    Mesh *mesh = malloc(sizeof(Mesh));
    if (!mesh)
        return NULL;

    fastObjMesh* obj_mesh = fast_obj_read(filename);
    if (!obj_mesh)
        return NULL;

    mesh->num_triangles = obj_mesh->face_count;
    mesh->triangles_local = malloc(sizeof(Triangle) * mesh->num_triangles);
    mesh->triangles_transformed = malloc(sizeof(Triangle) * mesh->num_triangles);
    mesh->radius = 0.0;
    mesh->state = 1;

    for (unsigned int f = 0; f < obj_mesh->face_count; f++) {
        Triangle t = {};
        for (unsigned int v = 0; v < 3; v++) {
            fastObjIndex index = obj_mesh->indices[f * 3 + v];
            Vertex vertex = (Vertex) {
                .vertex = {
                    .x = (double) obj_mesh->positions[index.p * 3],
                    .y = (double) obj_mesh->positions[index.p * 3 + 1],
                    .z = (double) obj_mesh->positions[index.p * 3 + 2]
                },
                .normal = {
                    .x = (double) obj_mesh->normals[index.n * 3],
                    .y = (double) obj_mesh->normals[index.n * 3 + 1],
                    .z = (double) obj_mesh->normals[index.n * 3 + 2]
                },
                .uv = {
                    .x = (double) obj_mesh->texcoords[index.t * 3],
                    .y = (double) obj_mesh->texcoords[index.t * 3 + 1],
                }
            };
            double l;
            if ((l = vec3_length(vertex.vertex)) > mesh->radius) {
                mesh->radius = l;
            }
            t.vertices[v] = vertex;
        }

        t.normal = vec3_normalize(vec3_add(t.vertices[0].normal, vec3_add(t.vertices[1].normal, t.vertices[2].normal)));

        mesh->triangles_local[f] = t;
    }

    fast_obj_destroy(obj_mesh);

    memcpy(mesh->triangles_transformed, mesh->triangles_local, sizeof(Triangle) * mesh->num_triangles);

    mesh->world_transform = TRANSFORM_ZERO;

    return mesh;
}

static void render_line(Screen *screen, Vec2i from, Vec2i to, uint32_t c, bool clip)
{
    int dx = abs(to.x - from.x), sx = from.x < to.x ? 1 : -1;
    int dy = abs(to.y - from.y), sy = from.y < to.y ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    for (;;) {
        if (from.x >= 0 && from.x < screen->width && from.y >= 0 && from.y < screen->height) {
            int x = from.x - screen->height_2;
            int y = from.y - screen->height_2;
            if (!clip || sqrt(x * x + y * y) <= screen->height_2 - 2) {
                pixel(screen, from.x, from.y) = c;
            }
        }
        if (from.x == to.x && from.y == to.y) {
            break;
        }
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            from.x += sx;
        }
        if (e2 < dy) {
            err += dx;
            from.y += sy;
        }
    }
}

static void render_line_y(Screen *screen, int y, int x_from, int x_to, uint32_t color)
{
    if (y < 0 || y >= screen->height)
        return;

    if (x_from > x_to) {
        int x = x_from;
        x_from = x_to;
        x_to = x;
    }

    if (x_from < 0)
        x_from = 0;

    if (x_to > screen->width - 1)
        x_to = screen->width - 1;

    for (int x = x_from; x <= x_to; x++) {
        pixel(screen, x, y) = color;
    }
}

void renderer3d_compute_camera_transform(const Transform *world_transform, Transform *camera_transform, const Camera *camera)
{
    camera_transform->position = vec3_rotate_xy(
        vec3_subtract(world_transform->position, camera->transform.position),
        -camera->transform.rotation - M_PI_2 * 3
    );
    camera_transform->rotation = world_transform->rotation - camera->transform.rotation - M_PI_2 * 3;
}

bool renderer3d_is_transformed_mesh_in_viewport(const Mesh* mesh, const Camera *camera)
{
    // behind camera plane
    if (mesh->camera_transform.position.y + mesh->radius < camera->near_plane)
        return false;

    // outside camera cone
    double x = fabs(mesh->camera_transform.position.x);
    double theta = fabs(camera->fov) * 0.5 * DEG2RAD;
    double d_camera = mesh->camera_transform.position.y * tan(theta);
    double d_mesh = (x - d_camera) * cos(theta);
    if (d_mesh > mesh->radius)
        return false;

    return true;
}

void renderer3d_transform_mesh_triangles(const Mesh *mesh, const Camera *camera)
{
    for (int t = 0; t < mesh->num_triangles; t++) {
        for (int v = 0; v < 3; v++) {
            const Vec3 vertex_rotated = vec3_rotate_xy(mesh->triangles_local[t].vertices[v].vertex, mesh->world_transform.rotation - camera->transform.rotation);
            mesh->triangles_transformed[t].vertices[v].vertex.x = vertex_rotated.x * mesh->world_transform.scale + mesh->camera_transform.position.x;
            mesh->triangles_transformed[t].vertices[v].vertex.y = vertex_rotated.y * mesh->world_transform.scale + mesh->camera_transform.position.y;
            mesh->triangles_transformed[t].vertices[v].vertex.z = vertex_rotated.z * mesh->world_transform.scale + mesh->camera_transform.position.z;
            mesh->triangles_transformed[t].vertices[v].normal = vec3_rotate_xy(mesh->triangles_local[t].vertices[v].normal, mesh->world_transform.rotation - camera->transform.rotation);
            mesh->triangles_transformed[t].normal = vec3_rotate_xy(mesh->triangles_local[t].normal, mesh->world_transform.rotation - camera->transform.rotation);
        }
    }
}

static double viewport_k, viewport_k2;

/* Candidate for performance optimization */
static bool renderer3d_is_triangle_in_viewport(const Triangle *triangle)
{
    double max_y = 0;

    for (int i = 0; i < 3; i++) {
        double d_sq = triangle->vertices[i].vertex.x * triangle->vertices[i].vertex.x + triangle->vertices[i].vertex.z * triangle->vertices[i].vertex.z;
        double r_sq = viewport_k2 * triangle->vertices[i].vertex.y * triangle->vertices[i].vertex.y;

        // vertex is inside the circle
        if (d_sq <= r_sq)
            return true;

        if (triangle->vertices[i].vertex.y > max_y)
            max_y = triangle->vertices[i].vertex.y;
    }

    double radius = max_y * viewport_k;

    return is_outer_points_line_crossing_circle(triangle->vertices[0].vertex, triangle->vertices[1].vertex, radius)
        || is_outer_points_line_crossing_circle(triangle->vertices[1].vertex, triangle->vertices[2].vertex, radius)
        || is_outer_points_line_crossing_circle(triangle->vertices[0].vertex, triangle->vertices[2].vertex, radius);
}

int renderer3d_compute_render_list(RenderList *render_list, MeshList *mesh_list, Camera *camera)
{
    int num_triangles = 0;

    // required for discarding invisible triangles
    viewport_k = tan(camera->fov * 0.5 * DEG2RAD);
    viewport_k2 = viewport_k * viewport_k;

    for (int m = 0; m < mesh_list->num_meshes; m++) {
        if (!mesh_list->meshes[m]->state)
            continue;

        renderer3d_compute_camera_transform(&mesh_list->meshes[m]->world_transform, &mesh_list->meshes[m]->camera_transform, camera);

        if (!renderer3d_is_transformed_mesh_in_viewport(mesh_list->meshes[m], camera))
            continue;

        renderer3d_transform_mesh_triangles(mesh_list->meshes[m], camera);

        for (int t = 0; t < mesh_list->meshes[m]->num_triangles; t++) {
            Vec3 camera_normal = vec3_average(
                mesh_list->meshes[m]->triangles_transformed[t].vertices[0].vertex,
                mesh_list->meshes[m]->triangles_transformed[t].vertices[1].vertex,
                mesh_list->meshes[m]->triangles_transformed[t].vertices[2].vertex
            );

            // todo: additionally check if mesh is fully in viewport to skip per-triangle check
            if (!renderer3d_is_triangle_in_viewport(&mesh_list->meshes[m]->triangles_transformed[t]))
                continue;

            if (vec3_angle(camera_normal, mesh_list->meshes[m]->triangles_transformed[t].normal) <= M_PI_2) {
                continue;
            }

            render_list->triangles[num_triangles++] = &mesh_list->meshes[m]->triangles_transformed[t];
        }
    }

    render_list->num_triangles = num_triangles;

    return num_triangles;
}

static double viewport_d;

void renderer3d_rasterize_triangle_wireframe(Triangle *triangle, Screen *screen)
{
    Vec2i v1 = {(int) triangle->vertices[0].vertex.x, (int) triangle->vertices[0].vertex.y};
    Vec2i v2 = {(int) triangle->vertices[1].vertex.x, (int) triangle->vertices[1].vertex.y};
    Vec2i v3 = {(int) triangle->vertices[2].vertex.x, (int) triangle->vertices[2].vertex.y};

    render_line(screen, v1, v2, WIREFRAME_COLOR, false);
    render_line(screen, v2, v3, WIREFRAME_COLOR, false);
    render_line(screen, v1, v3, WIREFRAME_COLOR, false);
}

void renderer3d_rasterize_triangle_shaded(Triangle *triangle, Screen *screen)
{
    Col3i color = COL3I_BLACK;
    uint8_t light = (triangle->vertices[0].light + triangle->vertices[1].light + triangle->vertices[2].light) / 3;
    color.r += light;
    color.g += light;
    color.b += light;

    Vec2 vertices[3];

    for (int v = 0; v < 3; v++) {
        vertices[v] = (Vec2) {
            round(triangle->vertices[v].vertex.x),
            round(triangle->vertices[v].vertex.y)
        };
    }

    // sort by y
    Vec2 tmp;
    if (vertices[2].y < vertices[1].y) { tmp = vertices[2]; vertices[2] = vertices[1]; vertices[1] = tmp; }
    if (vertices[1].y < vertices[0].y) { tmp = vertices[1]; vertices[1] = vertices[0]; vertices[0] = tmp; }
    if (vertices[2].y < vertices[1].y) { tmp = vertices[2]; vertices[2] = vertices[1]; vertices[1] = tmp; }

    float dy_01 = (float) fabs(vertices[1].y - vertices[0].y);
    float dx_01 = (float) fabs(vertices[1].x - vertices[0].x);
    float dy_12 = (float) fabs(vertices[2].y - vertices[1].y);
    float dx_12 = (float) fabs(vertices[2].x - vertices[1].x);
    float dy_02 = (float) fabs(vertices[2].y - vertices[0].y);
    float dx_02 = (float) fabs(vertices[2].x - vertices[0].x);

    float s_1 = vertices[0].x < vertices[1].x ? 1.0f : -1.0f;
    float s_2 = vertices[0].x < vertices[2].x ? 1.0f : -1.0f;
    float s_12 = vertices[1].x < vertices[2].x ? 1.0f : -1.0f;

    uint32_t c = col3i_to_uint32_t(color);

    float x_from, x_to;

    int y_i = 0;

    // quick dirty code here...

    if (dy_01 > 0) {
        for (int y = vertices[0].y; y <= vertices[1].y; y++) {
            x_from = (float) vertices[0].x + (dx_02 / dy_02) * (float) y_i * s_2;
            x_to = (float) vertices[0].x + (dx_01 / dy_01) * (float) y_i * s_1;
            x_from = s_2 > 0 ? ceilf(x_from) : floorf(x_from);
            x_to = s_1 > 0 ? ceilf(x_to) : floorf(x_to);
            render_line_y(screen, y, (int) x_from, (int) x_to , c);
            y_i++;
        }
    } else {
        render_line_y(screen, vertices[0].y, vertices[0].x, vertices[1].x, c);
    }

    if (dy_12 > 0) {
        for (int y = vertices[1].y, y_i12 = 0; y <= vertices[2].y; y++) {
            x_from = (float) vertices[0].x + (dx_02 / dy_02) * (float) y_i * s_2;
            x_to = (float) vertices[1].x + (dx_12 / dy_12) * (float) y_i12 * s_12;
            x_from = s_2 < 0 ? ceilf(x_from) : floorf(x_from);
            x_to = s_12 < 0 ? ceilf(x_to) : floorf(x_to);
            render_line_y(screen, y, (int) x_from, (int) x_to , c);
            y_i++;
            y_i12++;
        }
    } else {
        render_line_y(screen, vertices[1].y, vertices[1].x, vertices[2].x, c);
    }
}

void renderer3d_compute_triangle_lighting(Triangle *triangle, const LightList *light_list)
{
    for (int l = 0; l < light_list->num_lights; l++) {
        for (int v = 0; v < 3; v++) {
            double angle = vec3_angle(light_list->lights[l]->normal, triangle->vertices[v].normal);
            triangle->vertices[v].light = (uint8_t) remap(M_PI_2, 0, 0, 255, angle);
        }
    }
}

void renderer3d_compute_triangle_for_rasterization(Triangle *triangle, Camera *camera, Screen *screen)
{
    for (int v = 0; v < 3; v++) {
        double x_screen = viewport_d * triangle->vertices[v].vertex.x / triangle->vertices[v].vertex.y + (double) screen->width_2;
        double y_screen = viewport_d * triangle->vertices[v].vertex.z / triangle->vertices[v].vertex.y + (double) screen->height_2;

        triangle->vertices[v].vertex.z = triangle->vertices[v].vertex.y; // used as w
        triangle->vertices[v].vertex.x = x_screen;
        triangle->vertices[v].vertex.y = y_screen;
        triangle->vertices[v].w_1 = 1.0 / triangle->vertices[v].vertex.z;
    }
}

void renderer3d_render(const RenderList *render_list, LightList *light_list, Camera *camera, Screen *screen)
{
    viewport_d = 1.0 / tan(camera->fov * DEG2RAD * 0.5);
    viewport_d *= screen->height_2;

    for (int l = 0; l < light_list->num_lights; l++) {
        renderer3d_compute_camera_transform(&light_list->lights[l]->world_transform, &light_list->lights[l]->camera_transform, camera);
        light_list->lights[l]->normal = (Vec3) {
            .x = -cos(light_list->lights[l]->camera_transform.rotation),
            .y = -sin(light_list->lights[l]->camera_transform.rotation),
            .z = 0
        };
    }

    for (int i = 0; i < render_list->num_triangles; i++) {
        renderer3d_compute_triangle_lighting(render_list->triangles[i], light_list);
        renderer3d_compute_triangle_for_rasterization(render_list->triangles[i], camera, screen);
        renderer3d_rasterize_triangle_shaded(render_list->triangles[i], screen);
        renderer3d_rasterize_triangle_wireframe(render_list->triangles[i], screen);
    }
}
