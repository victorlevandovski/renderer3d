#include "renderer3d_shader.h"

#include <stdint.h>

#include "stb_image.h"
#include "renderer3d.h"

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

uint32_t renderer3d_shader_shade(Fragment *fragment)
{
    Col3i color = COL3I_BLACK;
    color.r += fragment->light;
    color.g += fragment->light;
    color.b += fragment->light;
    return col3i_to_uint32_t(color);
}
