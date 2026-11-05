#include <stdlib.h>
#include <stdio.h>

#include "fenster.h"
#include "lib/fenster_ext.h"

#include "renderer3d.h"

#define SCREEN_W 800
#define SCREEN_H 800

int main()
{
    uint32_t buf[SCREEN_W * SCREEN_H];
    struct fenster f = { .title = "renderer3d", .width = SCREEN_W, .height = SCREEN_H, .buf = buf };
    fenster_open(&f);

    Screen screen;
    screen.buffer = f.buf;
    screen.width = SCREEN_W;
    screen.height = SCREEN_H;
    screen.half_width = SCREEN_W / 2;
    screen.half_height = SCREEN_H / 2;
    
    int accu = 0;
    int64_t last = 0;
    int64_t prev = fenster_time();
    char text[32];

    Mesh *mesh = renderer3d_load_mesh("assets/sphere.obj");

    while (fenster_loop(&f) == 0) {
        int64_t now = fenster_time();
        double delta_time = (double) (now - prev) * 0.001;

        if (f.keys[27])
            break;

        memset(f.buf, 0, f.width * f.height * sizeof(uint32_t));

        mesh->rotation += delta_time * 0.1;
        if (mesh->rotation > M_PI * 2)
            mesh->rotation -= M_PI * 2;

        renderer3d_render_mesh(mesh, &screen);

        if (now - last > 999) {
            last = now;
            snprintf(text, sizeof(text), "FPS: %d", accu);
            accu = 0;
        }
        fenster_text(&f, 8, 8, text, 2, 0x666666);

        accu += 1;
        prev = now;
    }
    
    fenster_close(&f);
    
    return 0;
}
