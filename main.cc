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
            }
            else {
                hitable_pdf plight(light_shape, hrec.p);
                mixture_pdf p(&plight, srec.pdf_ptr);
                ray scattered = ray(hrec.p, p.generate(), r.time());
                float pdf_val = p.value(scattered.direction());
                delete srec.pdf_ptr;
                return emitted + srec.attenuation*hrec.mat_ptr->scattering_pdf(r, hrec, scattered)*color(scattered, world, light_shape, depth+1) / pdf_val;
            }
        }
        else
            return emitted;
    }
    else
        return vec3(0,0,0);
}

void cornell_box(hitable **scene, camera **cam, float aspect) {
    int i = 0;
    hitable **list = new hitable*[8];
    material *red = new lambertian( new constant_texture(vec3(0.65, 0.05, 0.05)) );
    material *white = new lambertian( new constant_texture(vec3(0.73, 0.73, 0.73)) );
    material *green = new lambertian( new constant_texture(vec3(0.12, 0.45, 0.15)) );
    material *light = new diffuse_light( new constant_texture(vec3(15, 15, 15)) );
    material *back_light_mat = new diffuse_light(new constant_texture(vec3(20, 20, 20)));
    list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, green));
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    list[i++] = new flip_normals(new xz_rect(214, 343, 227, 262, 554, light));
    list[i++] = new xy_rect(0, 555, 0, 555, -800, light);
    list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, white));
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, white));
    material *glass = new dielectric(1.5);
    list[i++] = new sphere(vec3(190, 90, 190),90 , glass);
    list[i++] = new translate(new rotate_y(
                    new box(vec3(0, 0, 0), vec3(165, 330, 165), white),  15), vec3(265,0,295));
    *scene = new hitable_list(list,i);
    vec3 lookfrom(278, 278, -800);
    vec3 lookat(278,278,0);
    float dist_to_focus = 10.0;
    float aperture = 0.0;
    float vfov = 40.0;
    *cam = new camera(lookfrom, lookat, vec3(0,1,0),
                      vfov, aspect, aperture, dist_to_focus, 0.0, 1.0);
}

int main() {
    const int nx = 800;
    const int ny = 900;
    const int ns = 50; // Samples per pixel - lower for real-time, increase for quality

    InitWindow(nx, ny, "Ray Tracing - Cornell Box");
    SetTargetFPS(60);

    // Create image buffer
    Image image = GenImageColor(nx, ny, BLACK);
    Texture2D texture = LoadTextureFromImage(image);

    hitable *world;
    camera *cam;
    float aspect = float(nx) / float(ny);
    cornell_box(&world, &cam, aspect);
    
    hitable *light_shape = new xz_rect(213, 343, 227, 332, 0, 0);

    hitable *light_shape_second = new xy_rect(0, 555, 0, 555, -800, 0);
    hitable *glass_sphere = new sphere(vec3(190, 90, 190), 90, 0);
    hitable *a[2];

    a[0] = light_shape;
    a[1] = light_shape_second;
//    a[1] = glass_sphere;
    hitable_list hlist(a,2);

    auto start_time = std::chrono::high_resolution_clock::now();

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
            
            // Flip vertically for image
            ImageDrawPixel(&image, i, ny - 1 - j, (Color){(unsigned char)ir, (unsigned char)ig, (unsigned char)ib, 255});
        }
    }

    UnloadTexture(texture);
    texture = LoadTextureFromImage(image);

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
        DrawText("Ray Tracing - Cornell Box", 10, 10, 20, WHITE);
        DrawText("Press ESC to exit", 10, 40, 20, WHITE);
        EndDrawing();
    }

    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();

    return 0;
}

