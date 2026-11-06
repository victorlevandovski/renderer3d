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

static void render_line(Screen *screen, Vec2i from, Vec2i to, uint32_t c)
{
    int dx = abs(to.x - from.x), sx = from.x < to.x ? 1 : -1;
    int dy = abs(to.y - from.y), sy = from.y < to.y ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    for (;;) {
        if (from.x >= 0 && from.x < screen->width && from.y >= 0 && from.y < screen->height) {
            pixel(screen, from.x, from.y) = c;
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

void renderer3d_transform_mesh_triangles(Mesh *mesh)
{
    for (int i = 0; i < mesh->num_triangles; i++) {
        for (int j = 0; j < 3; j++) {
            Vec3 v = vec3_rotate_z(mesh->triangles_local[i].vertices[j].vertex, mesh->transform.rotation);
            mesh->triangles_transformed[i].vertices[j].vertex.x = v.x * mesh->transform.scale + mesh->transform.position.x;
            mesh->triangles_transformed[i].vertices[j].vertex.y = v.y * mesh->transform.scale + mesh->transform.position.y;
            mesh->triangles_transformed[i].vertices[j].vertex.z = v.z * mesh->transform.scale + mesh->transform.position.z;
            mesh->triangles_transformed[i].normal = vec3_rotate_z(mesh->triangles_local[i].normal, mesh->transform.rotation);
        }
    }
}

int renderer3d_compute_render_list(RenderList *render_list, MeshList *mesh_list, Camera *camera)
{
    int num_triangles = 0;

    for (int m = 0; m < mesh_list->num_meshes; m++) {
        if (!mesh_list->meshes[m]->state)
            continue;

        renderer3d_transform_mesh_triangles(mesh_list->meshes[m]);

        for (int t = 0; t < mesh_list->meshes[m]->num_triangles; t++) {
            Vec3 triangle_average = vec3_average(
                mesh_list->meshes[m]->triangles_transformed[t].vertices[0].vertex,
                mesh_list->meshes[m]->triangles_transformed[t].vertices[1].vertex,
                mesh_list->meshes[m]->triangles_transformed[t].vertices[2].vertex
            );
            Vec3 camera_normal = {
                .x = camera->transform.position.x + triangle_average.x,
                .y = camera->transform.position.y + triangle_average.y,
                .z = camera->transform.position.z + triangle_average.z
            };
            if (vec3_angle(camera_normal, mesh_list->meshes[m]->triangles_transformed[t].normal) > M_PI_2) {
                render_list->triangles[num_triangles++] = &mesh_list->meshes[m]->triangles_transformed[t];
            }
        }
    }

    render_list->num_triangles = num_triangles;

    return num_triangles;
}

void renderer3d_render(RenderList *render_list, Screen *screen)
{
    double f = 1000;

    for (int i = 0; i < render_list->num_triangles; i++) {
        Vec2i v1 = {
            f * render_list->triangles[i]->vertices[0].vertex.x / render_list->triangles[i]->vertices[0].vertex.y + screen->width_2,
            f * render_list->triangles[i]->vertices[0].vertex.z / render_list->triangles[i]->vertices[0].vertex.y + screen->height_2
        };
        Vec2i v2 = {
            f * render_list->triangles[i]->vertices[1].vertex.x / render_list->triangles[i]->vertices[1].vertex.y + screen->width_2,
            f * render_list->triangles[i]->vertices[1].vertex.z / render_list->triangles[i]->vertices[1].vertex.y + screen->height_2
        };
        Vec2i v3 = {
            f * render_list->triangles[i]->vertices[2].vertex.x / render_list->triangles[i]->vertices[2].vertex.y + screen->width_2,
            f * render_list->triangles[i]->vertices[2].vertex.z / render_list->triangles[i]->vertices[2].vertex.y + screen->height_2
        };

        render_line(screen, v1, v2, WIREFRAME_COLOR);
        render_line(screen, v2, v3, WIREFRAME_COLOR);
        render_line(screen, v1, v3, WIREFRAME_COLOR);
    }
}
