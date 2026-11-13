#pragma once

#include <stdint.h>
#include "lib/lib_math.h"

typedef struct {
    int x;
    int y;
    int w;
} FragmentVertex;

typedef struct {

    int x;
    int y;
    uint8_t light;
} Fragment;

typedef struct {
    uint32_t *data;
    unsigned int width;
    unsigned int height;
} Bitmap;

typedef struct {
    Bitmap bitmap;
} Texture;

Texture* renderer3d_shader_load_texture(char* filename);
uint32_t renderer3d_shader_shade(Fragment *fragment);
