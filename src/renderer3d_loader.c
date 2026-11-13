#include "renderer3d.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fast_obj.h"
#include "stb_image.h"

Object* renderer3d_load_object(char* filename)
{
    Mesh *mesh = renderer3d_load_mesh(filename);
    if (!mesh)
        return NULL;

    Object *object = malloc(sizeof(Object));
    if (!object) {
        free(mesh);
        return NULL;
    }

    object->mesh = mesh;
    object->lod_meshes = NULL;
    object->num_lod_meshes = 0;
    object->world_transform = TRANSFORM_ZERO;
    object->camera_transform = TRANSFORM_ZERO;
    object->radius = mesh->radius;
    object->state = 1;
    object->attributes = 0;

    return object;
}

Mesh* renderer3d_load_mesh(char* filename)
{
    fastObjMesh* obj_mesh = fast_obj_read(filename);
    if (!obj_mesh)
        return NULL;

    Mesh *mesh = malloc(sizeof(Mesh));
    if (!mesh) {
        fast_obj_destroy(obj_mesh);
        return NULL;
    }

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
                .normal = {
                    .x = (double) obj_mesh->normals[index.n * 3],
                    .y = (double) obj_mesh->normals[index.n * 3 + 1],
                    .z = (double) obj_mesh->normals[index.n * 3 + 2]
                },
                .uv = {
                    .x = (double) obj_mesh->texcoords[index.t * 2],
                    .y = (double) obj_mesh->texcoords[index.t * 2 + 1],
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

    return mesh;
}

Texture* renderer3d_shader_load_texture(char* filename)
{
    int width, height, n;
    unsigned char *data = stbi_load(filename, &width, &height, &n, 0);

    if (data == NULL)
        return NULL;

    Texture *texture = malloc(sizeof(Texture));

    Bitmap bitmap;
    bitmap.data = malloc(sizeof(uint32_t) * width * height);
    bitmap.width = width;
    bitmap.height = height;

    for (int i = 0; i < width * height; i++) {
        int index = i * n;
        uint8_t r = data[index + 0];
        uint8_t g = data[index + 1];
        uint8_t b = data[index + 2];
        //uint8_t a = data[index + 3];
        uint32_t color = ((uint32_t) r << 16) | ((uint32_t) g << 8) | (uint32_t) b;
        bitmap.data[i] = color;
    }

    texture->bitmap = bitmap;

    stbi_image_free(data);

    return texture;
}
