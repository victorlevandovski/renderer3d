#include "renderer3d_shader.h"

#include <stdint.h>

#include "renderer3d.h"

uint32_t renderer3d_shader_shade(Fragment *fragment)
{
    Col3i color = COL3I_BLACK;
    color.r += fragment->light;
    color.g += fragment->light;
    color.b += fragment->light;
    return col3i_to_uint32_t(color);
}
