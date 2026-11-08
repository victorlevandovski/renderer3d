#include <stdlib.h>
#include <string.h>

#include "fast_obj.h"

#include "renderer3d.h"

#include <math.h>

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

    mesh->transform = TRANSFORM_ZERO;

    return mesh;
}

#define pixel(s, x, y) ((s)->buffer[(((s)->height - y - 1) * (s)->width) + (x)])

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

static void render_circle(Screen *screen, Vec2i center, int radius, uint32_t c)
{
    int segments = 360;
    double angle = 0;

    for (int i = 1; i < segments; i++) {
        double next_angle = 2 * M_PI * i / segments;
        Vec2i from = {
            .x = (int) (cos(angle) * radius) + center.x,
            .y = (int) (sin(angle) * radius) + center.y,
        };
        Vec2i to = {
            .x = (int) (cos(next_angle) * radius) + center.x,
            .y = (int) (sin(next_angle) * radius) + center.y,
        };
        render_line(screen, from, to, c, false);
        angle = next_angle;
    }
}

void renderer3d_compute_mesh_camera_transform(Mesh *mesh, Camera *camera)
{
    Vec3 relative_position = vec3_subtract(mesh->transform.position, camera->transform.position);
    mesh->transform_camera.position = vec3_rotate_xy(relative_position, -camera->transform.rotation - M_PI_2 * 3);
}

bool renderer3d_is_mesh_in_viewport(Mesh* mesh, Camera *camera)
{
    // behind camera plane
    if (mesh->transform_camera.position.y + mesh->radius < camera->near_plane)
        return false;

    // outside camera cone
    double x = fabs(mesh->transform_camera.position.x);
    double theta = fabs(camera->fov) * 0.5 * DEG2RAD;
    double d_camera = mesh->transform_camera.position.y * tan(theta);
    double d_mesh = (x - d_camera) * cos(theta);
    if (d_mesh > mesh->radius)
        return false;

    return true;
}

void renderer3d_transform_mesh_triangles(Mesh *mesh, Camera *camera)
{
    for (int i = 0; i < mesh->num_triangles; i++) {
        for (int j = 0; j < 3; j++) {
            Vec3 v = vec3_rotate_xy(mesh->triangles_local[i].vertices[j].vertex, mesh->transform.rotation - camera->transform.rotation);
            mesh->triangles_transformed[i].vertices[j].vertex.x = v.x * mesh->transform.scale + mesh->transform_camera.position.x;
            mesh->triangles_transformed[i].vertices[j].vertex.y = v.y * mesh->transform.scale + mesh->transform_camera.position.y;
            mesh->triangles_transformed[i].vertices[j].vertex.z = v.z * mesh->transform.scale + mesh->transform_camera.position.z;
            mesh->triangles_transformed[i].normal = vec3_rotate_xy(mesh->triangles_local[i].normal, mesh->transform.rotation - camera->transform.rotation);
        }
    }
}

static double viewport_k, viewport_k2;

bool renderer3d_is_line_crossing_circle(double radius, Vec3 a, Vec3 b)
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

/* Candidate for performance optimization */
bool renderer3d_is_triangle_in_viewport(Triangle *triangle)
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

    return renderer3d_is_line_crossing_circle(radius, triangle->vertices[0].vertex, triangle->vertices[1].vertex)
        || renderer3d_is_line_crossing_circle(radius, triangle->vertices[1].vertex, triangle->vertices[2].vertex)
        || renderer3d_is_line_crossing_circle(radius, triangle->vertices[0].vertex, triangle->vertices[2].vertex);
}

int renderer3d_compute_render_list(RenderList *render_list, MeshList *mesh_list, Camera *camera)
{
    int num_triangles = 0;

    viewport_k = tan(camera->fov * 0.5 * DEG2RAD);
    viewport_k2 = viewport_k * viewport_k;

    for (int m = 0; m < mesh_list->num_meshes; m++) {
        if (!mesh_list->meshes[m]->state)
            continue;

        renderer3d_compute_mesh_camera_transform(mesh_list->meshes[m], camera);

        if (!renderer3d_is_mesh_in_viewport(mesh_list->meshes[m], camera))
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

void renderer3d_render(RenderList *render_list, Camera *camera, Screen *screen)
{
    double d = 1.0 / tan(camera->fov * DEG2RAD * 0.5);
    d *= screen->height_2;

    for (int i = 0; i < render_list->num_triangles; i++) {
        Vec2i v1 = {
            d * render_list->triangles[i]->vertices[0].vertex.x / render_list->triangles[i]->vertices[0].vertex.y + screen->width_2,
            d * render_list->triangles[i]->vertices[0].vertex.z / render_list->triangles[i]->vertices[0].vertex.y + screen->height_2
        };
        Vec2i v2 = {
            d * render_list->triangles[i]->vertices[1].vertex.x / render_list->triangles[i]->vertices[1].vertex.y + screen->width_2,
            d * render_list->triangles[i]->vertices[1].vertex.z / render_list->triangles[i]->vertices[1].vertex.y + screen->height_2
        };
        Vec2i v3 = {
            d * render_list->triangles[i]->vertices[2].vertex.x / render_list->triangles[i]->vertices[2].vertex.y + screen->width_2,
            d * render_list->triangles[i]->vertices[2].vertex.z / render_list->triangles[i]->vertices[2].vertex.y + screen->height_2
        };

        render_line(screen, v1, v2, WIREFRAME_COLOR, true);
        render_line(screen, v2, v3, WIREFRAME_COLOR, true);
        render_line(screen, v1, v3, WIREFRAME_COLOR, true);
    }

    //render_circle(screen, (Vec2i) {screen->width_2 - 0, screen->height_2 - 0}, screen->height_2 - 2, 0x202020);
}
