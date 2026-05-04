#ifdef _MSC_VER
#endif
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include "raylib.h"
#include <random>
#include <omp.h>
#include <chrono>

inline double drand48() {
    thread_local std::mt19937 generator(std::random_device{}());
    thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}
#include "sphere.h"
#include "hitable_list.h"
#include <cfloat>
#include "camera.h"
#include "material.h"
#include "box.h"
#include "aarect.h"
#include "texture.h"
#include "pdf.h"
#include "cylinder.h"

void write_csv(const std::string& filename, const std::vector<std::vector<double>>& data) {
    std::ofstream file(filename);
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i != row.size() - 1) file << ",";
        }
        file << "\n";
    }
}

inline vec3 de_nan(const vec3& c) {
    vec3 temp = c;
    if (!(temp[0] == temp[0])) temp[0] = 0;
    if (!(temp[1] == temp[1])) temp[1] = 0;
    if (!(temp[2] == temp[2])) temp[2] = 0;
    return temp;
}

vec3 color(const ray& r, hitable *world, hitable *light_shape, int depth) {
    hit_record hrec;
    if (world->hit(r, 0.001, FLT_MAX, hrec)) {
        scatter_record srec;
        vec3 emitted = hrec.mat_ptr->emitted(r, hrec, hrec.u, hrec.v, hrec.p);
        if (depth < 50 && hrec.mat_ptr->scatter(r, hrec, srec)) {
            if (srec.is_specular) {
                return srec.attenuation * color(srec.specular_ray, world, light_shape, depth+1);
            } else {
                hitable_pdf plight(light_shape, hrec.p);
                mixture_pdf p(&plight, srec.pdf_ptr);
                ray scattered = ray(hrec.p, p.generate(), r.time());
                float pdf_val = p.value(scattered.direction());
                delete srec.pdf_ptr;
                return emitted + srec.attenuation * hrec.mat_ptr->scattering_pdf(r, hrec, scattered) *
                                 color(scattered, world, light_shape, depth+1) / pdf_val;
            }
        } else
            return emitted;
    } else {
        vec3 unit_dir = unit_vector(r.direction());
        float t = 0.5f * (unit_dir.y() + 1.0f);
        return (1.0f - t) * vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
    }
}

void outdoor_scene(hitable **scene, camera **cam, float aspect) {
    int i = 0;
    hitable **list = new hitable*[5];

    material *green_ground = new lambertian(new constant_texture(vec3(0.2, 0.6, 0.1)));
    material *sun_light    = new diffuse_light(new constant_texture(vec3(40, 40, 35)));
    material *ambient_light = new diffuse_light(new constant_texture(vec3(3, 3, 4)));
    material *red_ball     = new lambertian(new constant_texture(vec3(0.8, 0.1, 0.1)));
    material *blue_cyl     = new lambertian(new constant_texture(vec3(0.1, 0.2, 0.9)));

    list[i++] = new xz_rect(-2000, 2000, -2000, 2000, 0, green_ground);
    list[i++] = new sphere(vec3(300, 500, -200), 150, sun_light);
    list[i++] = new flip_normals(new sphere(vec3(0, 800, 0), 500, ambient_light));
    list[i++] = new sphere(vec3(200, 60, 100), 60, red_ball);
    list[i++] = new cylinder(vec3(-100, 0, 150), 50, 120, blue_cyl);

    *scene = new hitable_list(list, i);

    vec3 lookfrom(0, 300, 600);
    vec3 lookat(0, 100, 0);
    float dist_to_focus = (lookfrom - lookat).length();
    *cam = new camera(lookfrom, lookat, vec3(0, 1, 0), 60.0, aspect, 0.0, dist_to_focus, 0.0, 1.0);
}

int main() {
    const int nx = 800;
    const int ny = 900;
    const int ns = 20;

    InitWindow(nx, ny, "Ray Tracing - Outdoor Scene");
    SetTargetFPS(60);

    Image image = GenImageColor(nx, ny, BLACK);
    Texture2D texture = LoadTextureFromImage(image);

    hitable *world;
    camera *cam;
    float aspect = float(ny) / float(nx);
    outdoor_scene(&world, &cam, aspect);

    material *dummy_mat = new lambertian(new constant_texture(vec3(0,0,0)));
    hitable *sun_shape     = new sphere(vec3(300, 500, -200), 150, dummy_mat);
    hitable *ambient_shape = new flip_normals(new sphere(vec3(0, 800, 0), 500, dummy_mat));
    hitable *a[2];
    a[0] = sun_shape;
    a[1] = ambient_shape;
    hitable_list hlist(a, 2);

    auto start_time = std::chrono::high_resolution_clock::now();

    Color *pixels = (Color *)image.data;

    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            vec3 col(0, 0, 0);
            for (int s = 0; s < ns; s++) {
                float u = float(i + drand48()) / float(nx);
                float v = float(j + drand48()) / float(ny);
                ray r = cam->get_ray(u, v);
                col += de_nan(color(r, world, &hlist, 0));
            }
            col /= float(ns);
            col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

            int ir = int(255.99 * col[0]);
            int ig = int(255.99 * col[1]);
            int ib = int(255.99 * col[2]);

            // ИСПРАВЛЕНО: переворот по вертикали
            pixels[(ny - 1 - j) * nx + i] = (Color){ (unsigned char)ir, (unsigned char)ig, (unsigned char)ib, 255 };
        }
    }

    UpdateTexture(texture, pixels);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Rendering time: " << duration.count() << " ms" << std::endl;
    double duration_timestamp = duration.count();

    std::vector<std::vector<double>> results_timestamp = {
        {nx, ny, ns, duration_timestamp}
    };
    write_csv("raytracing_parallel.csv", results_timestamp);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(texture, 0, 0, WHITE);
        DrawText("Ray Tracing - Outdoor Scene", 10, 10, 20, WHITE);
        DrawText("Press ESC to exit", 10, 40, 20, WHITE);
        EndDrawing();
    }

    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();

    return 0;
}
