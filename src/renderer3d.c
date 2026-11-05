#include <stdlib.h>
#include <string.h>

#include "fast_obj.h"

#include "renderer3d.h"

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
            };
            double l;
            if ((l = vec3_length(vertex.vertex)) > mesh->radius) {
                mesh->radius = l;
            }
            t.vertices[v] = vertex;
        }

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

void renderer3d_render_mesh(Mesh *mesh, Screen *screen)
{
    double f = 1000;

    for (int i = 0; i < mesh->num_triangles; i++) {
        for (int j = 0; j < 3; j++) {
            Vec2 xy = {mesh->triangles_local[i].vertices[j].vertex.x, mesh->triangles_local[i].vertices[j].vertex.y};
            xy = vec2_rotate(xy, mesh->transform.rotation);
            mesh->triangles_transformed[i].vertices[j].vertex.x = xy.x * mesh->transform.scale + mesh->transform.position.x;
            mesh->triangles_transformed[i].vertices[j].vertex.y = xy.y * mesh->transform.scale + mesh->transform.position.y;
            mesh->triangles_transformed[i].vertices[j].vertex.z = mesh->triangles_local[i].vertices[j].vertex.z * mesh->transform.scale + mesh->transform.position.z;
        }

        Vec2i v1 = {
            f * mesh->triangles_transformed[i].vertices[0].vertex.x / mesh->triangles_transformed[i].vertices[0].vertex.y + screen->width_2,
            f * mesh->triangles_transformed[i].vertices[0].vertex.z / mesh->triangles_transformed[i].vertices[0].vertex.y + screen->height_2
        };
        Vec2i v2 = {
            f * mesh->triangles_transformed[i].vertices[1].vertex.x / mesh->triangles_transformed[i].vertices[1].vertex.y + screen->width_2,
            f * mesh->triangles_transformed[i].vertices[1].vertex.z / mesh->triangles_transformed[i].vertices[1].vertex.y + screen->height_2
        };
        Vec2i v3 = {
            f * mesh->triangles_transformed[i].vertices[2].vertex.x / mesh->triangles_transformed[i].vertices[2].vertex.y + screen->width_2,
            f * mesh->triangles_transformed[i].vertices[2].vertex.z / mesh->triangles_transformed[i].vertices[2].vertex.y + screen->height_2
        };

        render_line(screen, v1, v2, 0x669900);
        render_line(screen, v2, v3, 0x669900);
        render_line(screen, v1, v3, 0x669900);
    }
}