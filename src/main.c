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
    char label_fps[32];
    char label_stats[64];
    char label_debug[128];

    Texture *mars_texture = renderer3d_shader_load_texture("assets/textures/t_mars.png");

    Mesh *cube_mesh = renderer3d_load_mesh("assets/cube.obj");
    cube_mesh->world_transform.position.x = 5;
    cube_mesh->world_transform.position.y = 0;
    cube_mesh->world_transform.scale = 1;
    cube_mesh->state = 0;

    Mesh *sphere_mesh = renderer3d_load_mesh("assets/sphere.obj");
    sphere_mesh->world_transform.position.x = 3;
    sphere_mesh->world_transform.position.y = 0;
    sphere_mesh->world_transform.scale = 1;
    sphere_mesh->state = 1;

    Mesh *complex_mesh = renderer3d_load_mesh("assets/torus.obj");
    complex_mesh->world_transform.position.x = 5;
    complex_mesh->world_transform.position.y = 0;
    complex_mesh->world_transform.scale = 1;
    complex_mesh->state = 0;

    MeshList mesh_list = {
        .meshes = {cube_mesh, sphere_mesh, complex_mesh},
        .num_meshes = 3
    };

    Light sun = {
        .world_transform = TRANSFORM_ZERO,
        .brightness = 1.0,
        .state = 1
    };

    LightList light_list = {
        .lights = {&sun},
        .num_lights = 1
    };

    Camera camera = {
        .transform = TRANSFORM_ZERO,
        .fov = 33.0,
        .near_plane = 0.1,
    };

    sun.world_transform.rotation = M_PI_2 * 0.5;
    camera.transform.rotation = 0.0;

    double camera_speed = 1.0;
    double camera_rotation_speed = 0.5; // rad/s

    RenderList *render_list = malloc(sizeof(RenderList));

    renderer3d_init(&screen, mars_texture);

    while (fenster_loop(&f) == 0) {
        int64_t now = fenster_time();
        double delta_time = (double) (now - prev) * 0.001;

        if (f.keys[27])
            break;

        if (f.keys[87]) {
            camera.transform.position.x += delta_time * camera_speed * cos(camera.transform.rotation);
            camera.transform.position.y += delta_time * camera_speed * sin(camera.transform.rotation);
        }
        if (f.keys[83]) {
            camera.transform.position.x -= delta_time * camera_speed * cos(camera.transform.rotation);
            camera.transform.position.y -= delta_time * camera_speed * sin(camera.transform.rotation);
        }
        if (f.keys[65]) {
            camera.transform.position.x += delta_time * camera_speed * cos(camera.transform.rotation + M_PI_2);
            camera.transform.position.y += delta_time * camera_speed * sin(camera.transform.rotation + M_PI_2);
        }
        if (f.keys[68]) {
            camera.transform.position.x -= delta_time * camera_speed * cos(camera.transform.rotation + M_PI_2);
            camera.transform.position.y -= delta_time * camera_speed * sin(camera.transform.rotation + M_PI_2);
        }

        if (f.keys[81]) camera.transform.rotation += delta_time * camera_rotation_speed;
        if (f.keys[69]) camera.transform.rotation -= delta_time * camera_rotation_speed;

        memset(f.buf, 0, f.width * f.height * sizeof(uint32_t));

        cube_mesh->world_transform.rotation += delta_time * 0.5;
        sphere_mesh->world_transform.rotation -= delta_time * 0.5;
        complex_mesh->world_transform.rotation += delta_time * 0.5;

        renderer3d_compute_render_list(render_list, &mesh_list, &camera);
        renderer3d_render(render_list, &light_list, &camera, &screen);

        snprintf(label_stats, sizeof(label_stats), "TRIS: %d", render_list->num_triangles);
        fenster_text(&f, 8, 24, label_stats, 2, 0x666666);

        snprintf(label_debug, sizeof(label_debug), "CAM  P: %.1f %.1f  R: %.1f", camera.transform.position.x, camera.transform.position.y, camera.transform.rotation);
        fenster_text(&f, 8, 48, label_debug, 2, 0x666666);

        if (now - last > 999) {
            last = now;
            snprintf(label_fps, sizeof(label_fps), "FPS: %d", accu);
            accu = 0;
        }
        fenster_text(&f, 8, 8, label_fps, 2, 0x666666);

        accu += 1;
        prev = now;
    }
    
    fenster_close(&f);
    
    return 0;
}
