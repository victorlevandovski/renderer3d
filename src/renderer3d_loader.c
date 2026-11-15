#include "renderer3d.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "fast_obj.h"
#include "stb_image.h"
#include "lib/lib_math.h"

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

    object->meshes[0] = mesh;

    char lod_filename[strlen(filename) + 7];
    char *dot = strrchr(filename, '.');

    for (int i = 1; i <= MESH_LOD_MAX_LEVELS; i++) {
        snprintf(lod_filename, sizeof(lod_filename), "%.*s_lod_%d.obj", (int) (dot - filename), filename, i);
        object->meshes[i] = renderer3d_load_mesh(lod_filename);
    }

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
        uint8_t a = n > 3 ? data[index + 3] : 255;
        uint32_t color = ((uint32_t) a << 24) | ((uint32_t) r << 16) | ((uint32_t) g << 8) | (uint32_t) b;
        bitmap.data[i] = color;
    }

    texture->bitmaps[0] = bitmap;

    int mipmaps_count = (int) log2(maxi(width, height)) - 1;

    for (int i = 1; i <= mipmaps_count; i++) {
        Bitmap mipmap;
        unsigned int larger_width = texture->bitmaps[i - 1].width;
        unsigned int larger_height = texture->bitmaps[i - 1].height;
        mipmap.width = larger_width / 2;
        mipmap.height = larger_height / 2;
        mipmap.data = malloc(mipmap.width * mipmap.height * sizeof(uint32_t));

        for (int x = 0, mx = 0; mx < mipmap.width; x += 2, mx++) {
            for (int y = 0, my = 0; my < mipmap.height; y += 2, my++) {
                uint32_t c1 = texture->bitmaps[i - 1].data[y * larger_width + x];
                uint32_t c2 = texture->bitmaps[i - 1].data[y * larger_width + x + 1];
                uint32_t c3 = texture->bitmaps[i - 1].data[(y + 1) * larger_width + x];
                uint32_t c4 = texture->bitmaps[i - 1].data[(y + 1) * larger_width + x + 1];
                Col4i col1 = uint32_t_to_col4i(c1);
                Col4i col2 = uint32_t_to_col4i(c2);
                Col4i col3 = uint32_t_to_col4i(c3);
                Col4i col4 = uint32_t_to_col4i(c4);

                Col4i final = {
                    .r = (col1.r + col2.r + col3.r + col4.r) / 4,
                    .g = (col1.g + col2.g + col3.g + col4.g) / 4,
                    .b = (col1.b + col2.b + col3.b + col4.b) / 4,
                    .a = (col1.a + col2.a + col3.a + col4.a) / 4,
                };

                mipmap.data[my * mipmap.width + mx] = col4i_to_uint32_t(final);
            }
        }

        texture->bitmaps[i] = mipmap;
    }

    stbi_image_free(data);

    return texture;
}
