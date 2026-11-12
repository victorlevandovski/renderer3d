#include <stdlib.h>
#include <string.h>

#include "fast_obj.h"

#include "renderer3d.h"

#include <math.h>

Col3i COL3I_BLACK = {
    .r = 0,
    .g = 0,
    .b = 0,
};

Col3i COL3I_WHITE = {
    .r = 255,
    .g = 255,
    .b = 255,
};

Transform TRANSFORM_ZERO = {
    .position = (Vec3) { .x = 0.0, .y = 0.0, .z = 0.0 },
    .rotation = 0.0,
    .scale = 1.0,
};

void renderer3d_init(const Screen *screen)
{
    z_buffer = malloc(sizeof(double) * screen->width * screen->height);
}

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
    mesh->state = 1;

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

    mesh->world_transform = TRANSFORM_ZERO;

    return mesh;
}

static void render_line(Screen *screen, Vec2i from, Vec2i to, uint32_t c, bool clip)
{
    int dx = abs(to.x - from.x), sx = from.x < to.x ? 1 : -1;
    int dy = abs(to.y - from.y), sy = from.y < to.y ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    for (;;) {
        if (from.x >= 0 && from.x < screen->width && from.y >= 0 && from.y < screen->height) {
            int x = from.x - screen->height_2;
            int y = from.y - screen->height_2;
            if (!clip || sqrt(x * x + y * y) <= screen->height_2 - 2) {
                pixel(screen, from.x, from.y) = c;
            }
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

void renderer3d_compute_camera_transform(const Transform *world_transform, Transform *camera_transform, const Camera *camera)
{
    camera_transform->position = vec3_rotate_xy(
        vec3_subtract(world_transform->position, camera->transform.position),
        -camera->transform.rotation - M_PI_2 * 3
    );
    camera_transform->rotation = world_transform->rotation - camera->transform.rotation - M_PI_2 * 3;
}

bool renderer3d_is_transformed_mesh_in_viewport(const Mesh* mesh, const Camera *camera)
{
    // behind camera plane
    if (mesh->camera_transform.position.y + mesh->radius < camera->near_plane)
        return false;

    // outside camera cone
    double x = fabs(mesh->camera_transform.position.x);
    double theta = fabs(camera->fov) * 0.5 * DEG2RAD;
    double d_camera = mesh->camera_transform.position.y * tan(theta);
    double d_mesh = (x - d_camera) * cos(theta);
    if (d_mesh > mesh->radius)
        return false;

    return true;
}

void renderer3d_transform_mesh_triangles(const Mesh *mesh, const Camera *camera)
{
    for (int t = 0; t < mesh->num_triangles; t++) {
        for (int v = 0; v < 3; v++) {
            const Vec3 vertex_rotated = vec3_rotate_xy(mesh->triangles_local[t].vertices[v].vertex, mesh->world_transform.rotation - camera->transform.rotation);
            mesh->triangles_transformed[t].vertices[v].vertex.x = vertex_rotated.x * mesh->world_transform.scale + mesh->camera_transform.position.x;
            mesh->triangles_transformed[t].vertices[v].vertex.y = vertex_rotated.y * mesh->world_transform.scale + mesh->camera_transform.position.y;
            mesh->triangles_transformed[t].vertices[v].vertex.z = vertex_rotated.z * mesh->world_transform.scale + mesh->camera_transform.position.z;
            mesh->triangles_transformed[t].vertices[v].normal = vec3_rotate_xy(mesh->triangles_local[t].vertices[v].normal, mesh->world_transform.rotation - camera->transform.rotation);
            mesh->triangles_transformed[t].normal = vec3_rotate_xy(mesh->triangles_local[t].normal, mesh->world_transform.rotation - camera->transform.rotation);
        }
    }
}

static double viewport_k, viewport_k2;

/* Candidate for performance optimization */
static bool renderer3d_is_triangle_in_viewport(const Triangle *triangle)
{
    double max_y = 0;

    for (int i = 0; i < 3; i++) {
        double d_sq = triangle->vertices[i].vertex.x * triangle->vertices[i].vertex.x + triangle->vertices[i].vertex.z * triangle->vertices[i].vertex.z;
        double r_sq = viewport_k2 * triangle->vertices[i].vertex.y * triangle->vertices[i].vertex.y;

        // vertex is inside the circle
        if (d_sq <= r_sq)
            return true;

        if (triangle->vertices[i].vertex.y > max_y)
            max_y = triangle->vertices[i].vertex.y;
    }

    double radius = max_y * viewport_k;

    return is_outer_points_line_crossing_circle(triangle->vertices[0].vertex, triangle->vertices[1].vertex, radius)
        || is_outer_points_line_crossing_circle(triangle->vertices[1].vertex, triangle->vertices[2].vertex, radius)
        || is_outer_points_line_crossing_circle(triangle->vertices[0].vertex, triangle->vertices[2].vertex, radius);
}

int renderer3d_compute_render_list(RenderList *render_list, MeshList *mesh_list, Camera *camera)
{
    int num_triangles = 0;

    // required for discarding invisible triangles
    viewport_k = tan(camera->fov * 0.5 * DEG2RAD);
    viewport_k2 = viewport_k * viewport_k;

    for (int m = 0; m < mesh_list->num_meshes; m++) {
        if (!mesh_list->meshes[m]->state)
            continue;

        renderer3d_compute_camera_transform(&mesh_list->meshes[m]->world_transform, &mesh_list->meshes[m]->camera_transform, camera);

        if (!renderer3d_is_transformed_mesh_in_viewport(mesh_list->meshes[m], camera))
            continue;

        renderer3d_transform_mesh_triangles(mesh_list->meshes[m], camera);

        for (int t = 0; t < mesh_list->meshes[m]->num_triangles; t++) {
            Vec3 camera_normal = vec3_average(
                mesh_list->meshes[m]->triangles_transformed[t].vertices[0].vertex,
                mesh_list->meshes[m]->triangles_transformed[t].vertices[1].vertex,
                mesh_list->meshes[m]->triangles_transformed[t].vertices[2].vertex
            );

            // todo: additionally check if mesh is fully in viewport to skip per-triangle check
            if (!renderer3d_is_triangle_in_viewport(&mesh_list->meshes[m]->triangles_transformed[t]))
                continue;

            if (vec3_angle(camera_normal, mesh_list->meshes[m]->triangles_transformed[t].normal) <= M_PI_2) {
                continue;
            }

            render_list->triangles[num_triangles++] = &mesh_list->meshes[m]->triangles_transformed[t];
        }
    }

    render_list->num_triangles = num_triangles;

    return num_triangles;
}

static double viewport_d;

static void renderer3d_rasterize_triangle_wireframe(const Triangle *triangle, Screen *screen)
{
    Vec2i v1 = {(int) triangle->vertices[0].vertex.x, (int) triangle->vertices[0].vertex.y};
    Vec2i v2 = {(int) triangle->vertices[1].vertex.x, (int) triangle->vertices[1].vertex.y};
    Vec2i v3 = {(int) triangle->vertices[2].vertex.x, (int) triangle->vertices[2].vertex.y};

    render_line(screen, v1, v2, WIREFRAME_COLOR, false);
    render_line(screen, v2, v3, WIREFRAME_COLOR, false);
    render_line(screen, v1, v3, WIREFRAME_COLOR, false);
}

static double renderer3d_edge(const Vec3 a, const Vec3 b, const Vec3 c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static void rasterize_triangle_line(Triangle *triangle, Screen *screen, int y, int x_from, int x_to, uint32_t override_color)
{
    if (y < 0 || y >= screen->height) return;
    if (x_from < 0) x_from = 0;
    if (x_to > screen->width - 1) x_to = screen->width - 1;

    for (int x = x_from; x <= x_to; x++) {
        // Quick discard.
        // Todo: see if it helps or steals FPS when rendering actual models
        // Also we can assign relative z values to entire meshes, if they can be quickly sorted by depth + radius
        if (z_buffer[screen->width * y + x] > triangle->w_inv_max) continue;

        double weight_0, weight_1, weight_2;

        if (fabs(triangle->area) > 6.0) {
            Vec3 p = { (double) x, (double) y, 0.0 };
            weight_0 = renderer3d_edge(triangle->vertices[1].vertex, triangle->vertices[2].vertex, p) / triangle->area;
            weight_1 = renderer3d_edge(triangle->vertices[2].vertex, triangle->vertices[0].vertex, p) / triangle->area;
            weight_2 = renderer3d_edge(triangle->vertices[0].vertex, triangle->vertices[1].vertex, p) / triangle->area;
        } else {
            // simple approximation for small triangles
            weight_0 = weight_1 = weight_2 = M_1_3;
        }

        double p_w_inv = weight_0 * triangle->vertices[0].w_inv + weight_1 * triangle->vertices[1].w_inv + weight_2 * triangle->vertices[2].w_inv;
        p_w_inv = clamp(p_w_inv, 0.0, 1.0);

        if (z_buffer[screen->width * y + x] > p_w_inv) continue;

        double light = weight_0 * triangle->vertices[0].light + weight_1 * triangle->vertices[1].light + weight_2 * triangle->vertices[2].light;
        light = clamp(light, 0.0, 1.0);

        uint8_t light_8 = (uint8_t) (light * 255);
        light_8 = clamp(light_8, 0, 255);
        Col3i color = COL3I_BLACK;
        color.r = light_8;
        color.g = light_8;
        color.b = light_8;

        pixel(screen, x, y) = col3i_to_uint32_t(color);
        z_buffer[screen->width * y + x] = p_w_inv;
        // Fragment f = {
        //     .x = x,
        //     .y = y,
        //     .light = 128
        // };
        // pixel(screen, x, y) = renderer3d_shader_shade(&f);
    }
}

static void renderer3d_rasterize_triangle_shaded(Triangle *triangle, Screen *screen)
{
    // sort by y
    Vertex tmp;
    if (triangle->vertices[2].vertex.y < triangle->vertices[1].vertex.y) { swap(triangle->vertices[1], triangle->vertices[2], tmp); }
    if (triangle->vertices[1].vertex.y < triangle->vertices[0].vertex.y) { swap(triangle->vertices[0], triangle->vertices[1], tmp); }
    if (triangle->vertices[2].vertex.y < triangle->vertices[1].vertex.y) { swap(triangle->vertices[1], triangle->vertices[2], tmp); }

    triangle->area = renderer3d_edge(triangle->vertices[0].vertex, triangle->vertices[1].vertex, triangle->vertices[2].vertex);
    triangle->w_inv_max = fmax(triangle->vertices[0].w_inv, fmax(triangle->vertices[1].w_inv, triangle->vertices[1].w_inv));

    Vec2 vertices[3];

    for (int v = 0; v < 3; v++) {
        vertices[v] = (Vec2) {
            triangle->vertices[v].vertex.x,
            triangle->vertices[v].vertex.y
        };
    }

    int dy_01 = ((int) ceil(vertices[1].y) + 0) - (int) ceil(vertices[0].y);
    int dy_12 = ((int) ceil(vertices[2].y) + 0) - (int) ceil(vertices[1].y);

    double x_start, x_end, x_tmp;
    int y_i01 = 0;

    double dx_start = (vertices[2].x - vertices[0].x) / (vertices[2].y - vertices[0].y);

    uint32_t c = 0xffffff;
    uint32_t top_color = c;
    uint32_t bottom_color = c;
    uint32_t top_line_color = c;
    uint32_t bottom_line_color = c;

    if (dy_01 > 0) {
        double dx_end = (vertices[1].x - vertices[0].x) / (vertices[1].y - vertices[0].y);
        int y_start = (int) ceil(vertices[0].y);
        int y_end = (int) ceil(vertices[1].y) - 1;
        for (int y = y_start; y <= y_end; y++) {
            x_start = vertices[0].x + dx_start * y_i01;
            x_end = vertices[0].x + dx_end * y_i01;
            if (x_start > x_end) { swap(x_start, x_end, x_tmp); }
            rasterize_triangle_line(triangle, screen, y, (int) ceil(x_start), (int) ceil(x_end) - 1, top_color);
            y_i01++;
        }
    } else {
        //x_start = vertices[0].x;
        //x_end = vertices[1].x;
        //if (x_start > x_end) { swap(x_start, x_end, x_tmp); }
        //rasterize_triangle_line(triangle, screen, (int) ceil(vertices[0].y), (int) ceil(x_start), (int) ceil(x_end) - 1, top_line_color);
    }

    if (dy_12 > 0) {
        double dx_end = (vertices[2].x - vertices[1].x) / (vertices[2].y - vertices[1].y);
        int y_start = (int) ceil(vertices[1].y);
        int y_end = (int) ceil(vertices[2].y) - 1;
        for (int y = y_start, y_i12 = 0; y <= y_end; y++) {
            x_start = vertices[0].x + dx_start * y_i01;
            x_end = vertices[1].x + dx_end * y_i12;
            if (x_start > x_end) { swap(x_start, x_end, x_tmp); }
            rasterize_triangle_line(triangle, screen, y, (int) ceil(x_start), (int) ceil(x_end) - 1, bottom_color);
            y_i01++;
            y_i12++;
        }
    } else {
        x_start = vertices[1].x;
        x_end = vertices[2].x;
        if (x_start > x_end) { swap(x_start, x_end, x_tmp); }
        rasterize_triangle_line(triangle, screen, (int) ceil(vertices[1].y), (int) ceil(x_start), (int) ceil(x_end) - 1, bottom_line_color);
    }
}

void renderer3d_compute_triangle_lighting(Triangle *triangle, const LightList *light_list)
{
    for (int l = 0; l < light_list->num_lights; l++) {
        for (int v = 0; v < 3; v++) {
            double angle = vec3_angle(light_list->lights[l]->normal, triangle->vertices[v].normal);
            triangle->vertices[v].light = remap(0.0, M_PI_2, 1.0, 0.0, angle);
        }
    }
}

void renderer3d_compute_triangle_for_rasterization(Triangle *triangle, Camera *camera, Screen *screen)
{
    for (int v = 0; v < 3; v++) {
        double x_screen = viewport_d * triangle->vertices[v].vertex.x / triangle->vertices[v].vertex.y + (double) screen->width_2;
        double y_screen = viewport_d * triangle->vertices[v].vertex.z / triangle->vertices[v].vertex.y + (double) screen->height_2;

        triangle->vertices[v].vertex.z = triangle->vertices[v].vertex.y; // used as w
        triangle->vertices[v].vertex.x = x_screen;
        triangle->vertices[v].vertex.y = y_screen;
        triangle->vertices[v].w_inv = 1.0 / triangle->vertices[v].vertex.z;
    }
}

void renderer3d_render_triangle(Triangle *triangle, Screen *screen)
{
    renderer3d_rasterize_triangle_shaded(triangle, screen);
    //renderer3d_rasterize_triangle_wireframe(triangle, screen);
}

void renderer3d_render(const RenderList *render_list, LightList *light_list, Camera *camera, Screen *screen)
{
    memset(z_buffer, 0, screen->width * screen->height * sizeof(double));

    viewport_d = 1.0 / tan(camera->fov * DEG2RAD * 0.5);
    viewport_d *= screen->height_2;

    for (int l = 0; l < light_list->num_lights; l++) {
        renderer3d_compute_camera_transform(&light_list->lights[l]->world_transform, &light_list->lights[l]->camera_transform, camera);
        light_list->lights[l]->normal = (Vec3) {
            .x = -cos(light_list->lights[l]->camera_transform.rotation),
            .y = -sin(light_list->lights[l]->camera_transform.rotation),
            .z = 0
        };
    }

    for (int i = 0; i < render_list->num_triangles; i++) {
        renderer3d_compute_triangle_lighting(render_list->triangles[i], light_list);
        renderer3d_compute_triangle_for_rasterization(render_list->triangles[i], camera, screen);
        renderer3d_render_triangle(render_list->triangles[i], screen);
    }
}
