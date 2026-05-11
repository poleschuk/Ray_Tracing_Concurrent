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

vec3 SUN_POSITION = vec3(1000, 500, 300);
float SUN_R = 300;

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

/*
vec3 sky_with_sun(const ray& r, const vec3& light_center, const vec3& light_color) {
    vec3 dir = unit_vector(r.direction());
    float t = 0.5 * (dir.y() + 1.0);
    vec3 sky = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);

    // "Солнце" – яркое пятно в направлении на центр источника
    float cos_angle = dot(dir, unit_vector(light_center));
    float sun = pow(std::max(cos_angle, 0.0f), 500.0f);  // большое число – резкий диск
    sky += sun * light_color;  // добавляем излучение источника
    return sky;
}
*/

vec3 sky_with_sun(const ray& r,
                  const vec3& light_center,
                  float light_radius,
                  const vec3& light_color)
{
    vec3 dir = unit_vector(r.direction());
    float t = 0.5f * (dir.y() + 1.0f);           // для градиента неба
    vec3 sky = (1.0f - t) * vec3(1.0f, 1.0f, 1.0f)
             + t * vec3(0.5f, 0.7f, 1.0f);

    // Вектор от начала луча (камеры) до центра источника
    vec3 to_light = light_center - r.origin();
    float dist = to_light.length();
    // Если источник находится "за спиной" или сбоку, он не должен быть виден
    // (можно просто не учитывать, если направление сильно расходится)
    if (dot(dir, to_light) <= 0) return sky;   // источник позади

    // Синус углового радиуса (половина угла конуса)
    float sin_theta = light_radius / dist;
    if (sin_theta > 1.0f) {
        // Источник настолько близко, что охватывает всё небо -> возвращаем его яркость
        return light_color;
    }

    // Косинус угла между направлением луча и направлением на центр источника
    float cos_alpha = dot(dir, to_light / dist);
    // Косинус половины угла конуса видимости
    float cos_theta = sqrt(1.0f - sin_theta * sin_theta);

    if (cos_alpha >= cos_theta) {
        // Луч попадает в диск источника – возвращаем его цвет
        return light_color;
    } else {
        // Для плавного перехода (корона) можно добавить небольшое гало:
        float edge = 0.005f;  // ширина переходной зоны
        if (cos_alpha >= cos_theta - edge) {
            float mix = (cos_alpha - (cos_theta - edge)) / edge;
            return mix * light_color + (1.0f - mix) * sky;
        }
        return sky;
    }
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
        return vec3(0, 0, 0);
}

vec3 sky_color(const ray& r) {
    vec3 dir = unit_vector(r.direction());
    float t = 0.5 * (dir.y() + 1.0);
    return (1.0 - t) * vec3(1, 1, 1) + t * vec3(0.5, 0.7, 1);
}

vec3 constant_sky_color = vec3(0.5, 0.7, 1);

void cornell_box(hitable **scene, camera **cam, float aspect) {
    int i = 0;
    hitable **list = new hitable*[8];
    material *red = new lambertian( new constant_texture(vec3(0.65, 0.05, 0.05)) );
    material *white = new lambertian( new constant_texture(vec3(0.73, 0.73, 0.73)) );
    material *green = new lambertian( new constant_texture(vec3(0.12, 0.45, 0.15)) );
    material *light = new diffuse_light( new constant_texture(vec3(15, 15, 15)) );
    list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, green));
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    list[i++] = new flip_normals(new xz_rect(213, 343, 227, 332, 554, light));
    list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, white));
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, white));
    list[i++] = new sphere(vec3(190, 90, 190),90 , white);
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


void open_scene(hitable **scene, camera **cam, float aspect) {
    int i = 0;
    hitable **list = new hitable*[5];
    material *green = new lambertian(new constant_texture(vec3(0.12, 0.45, 0.15)));
    material *red = new lambertian(new constant_texture(vec3(0.65, 0.05, 0.05)));
    material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
    material *light = new diffuse_light(new constant_texture(vec3(5, 5, 5)));
    material *blue_sky = new lambertian(new constant_texture(constant_sky_color));
    list[i++] = new xz_rect(-1000, 1000, -1000, 1000, 0, green);
    list[i++] = new flip_normals(new xy_rect(-1000, 1000, -1000, 1000, -1000, blue_sky));
    list[i++] = new cylinder(vec3(-55, 0, 20), 80, 220, white);
    list[i++] = new sphere(SUN_POSITION, SUN_R, light);
    list[i++] = new sphere(vec3(130, 80.1, -300), 80, red);

    *scene = new hitable_list(list,i);
    vec3 lookfrom(0, 400, 800);
    vec3 lookat(0,0,0);
    float dist_to_focus = 10.0;
    float aperture = 0.0;
    float vfov = 40.0;
    *cam = new camera(lookfrom, lookat, vec3(0,1,0),
                      vfov, aspect, aperture, dist_to_focus, 0.0, 1.0);
}

int main() {
    const int nx = 800;
    const int ny = 900;
    const int ns = 100;

    InitWindow(nx, ny, "Ray Tracing - Cornell Box");
    SetTargetFPS(60);

    Image image = GenImageColor(nx, ny, DARKBLUE);
    Texture2D texture = LoadTextureFromImage(image);

    hitable *world;
    camera *cam;
    float aspect = float(nx) / float(ny);
    cornell_box(&world, &cam, aspect);
//    open_scene(&world, &cam, aspect);
//    hitable *light_sun = new sphere(SUN_POSITION, SUN_R, 0);
    hitable *light_box = new xz_rect(213, 343, 227, 332, 554, 0);


    hitable *a[1];
    a[0] = light_box;
    hitable_list hlist(a,1);

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

