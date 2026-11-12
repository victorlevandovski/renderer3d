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

uint32_t renderer3d_shader_shade(Fragment *fragment);
