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
    screen.width_2 = SCREEN_W / 2;
    screen.height_2 = SCREEN_H / 2;
    
    int accu = 0;
    int64_t last = 0;
    int64_t prev = fenster_time();
    char text[32];

    Mesh *cube_mesh = renderer3d_load_mesh("assets/cube.obj");
    cube_mesh->transform.position.x = 65;
    cube_mesh->transform.position.y = 400;
    cube_mesh->transform.scale = 100;
    cube_mesh->state = 1;

    Mesh *sphere_mesh = renderer3d_load_mesh("assets/sphere.obj");
    sphere_mesh->transform.position.x = -65;
    sphere_mesh->transform.position.y = 400;
    sphere_mesh->transform.scale = 100;
    sphere_mesh->state = 1;

    MeshList mesh_list = {
        .meshes = {cube_mesh, sphere_mesh},
        .num_meshes = 2
    };

    Camera camera = {
        .transform = TRANSFORM_ZERO,
        .focal_length = 300,
        .near_plane = 0.1,
        .far_plane = 1000,
    };

    camera.transform.rotation = M_PI_2;

    RenderList *render_list = malloc(sizeof(RenderList));

    while (fenster_loop(&f) == 0) {
        int64_t now = fenster_time();
        double delta_time = (double) (now - prev) * 0.001;

        if (f.keys[27])
            break;

        memset(f.buf, 0, f.width * f.height * sizeof(uint32_t));

        cube_mesh->transform.rotation += delta_time * 0.5;
        sphere_mesh->transform.rotation -= delta_time * 0.5;

        renderer3d_compute_render_list(render_list, &mesh_list, &camera);
        renderer3d_render(render_list, &screen);

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
