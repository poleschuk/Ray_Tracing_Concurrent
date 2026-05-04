#ifndef CYLINDERH
#define CYLINDERH

#include "hitable.h"

class cylinder : public hitable {
public:
    cylinder(const vec3& base_center, float r, float h, material* m)
        : center(base_center), radius(r), height(h), mat_ptr(m) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
    virtual bool bounding_box(float t0, float t1, aabb& box) const;

    vec3 center;    // центр нижнего основания
    float radius, height;
    material* mat_ptr;
};

bool cylinder::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
    // Ось цилиндра параллельна Y, центр основания в center, высота height.
    // Параметризуем луч, решаем уравнение бесконечного цилиндра.
    float ox = r.origin().x() - center.x();
    float oz = r.origin().z() - center.z();
    float dx = r.direction().x();
    float dz = r.direction().z();

    float a = dx*dx + dz*dz;
    float b = 2.0f*(ox*dx + oz*dz);
    float c = ox*ox + oz*oz - radius*radius;

    float closest_t = t_max;
    bool hit_anything = false;

    // Бесконечный цилиндр
    if (a > 1e-6f) {
        float disc = b*b - 4.0f*a*c;
        if (disc >= 0) {
            float sqrt_disc = sqrt(disc);
            float t0 = (-b - sqrt_disc) / (2.0f*a);
            float t1 = (-b + sqrt_disc) / (2.0f*a);

            if (t0 > t1) std::swap(t0, t1);

            // Проверяем попадание в диапазон высот
            float y0 = r.origin().y() + t0*r.direction().y();
            if (t0 > t_min && t0 < closest_t && y0 >= center.y() && y0 <= center.y()+height) {
                closest_t = t0;
                hit_anything = true;
                rec.t = t0;
                rec.p = r.point_at_parameter(t0);
                vec3 outward_normal = (rec.p - vec3(center.x(), rec.p.y(), center.z())) / radius;
                rec.normal = outward_normal;
                rec.mat_ptr = mat_ptr;
                // u,v можно задать простенько, если не нужны текстуры
                rec.u = atan2(rec.p.z()-center.z(), rec.p.x()-center.x()) / (2*M_PI);
                rec.v = (rec.p.y()-center.y()) / height;
            }

            float y1 = r.origin().y() + t1*r.direction().y();
            if (t1 > t_min && t1 < closest_t && y1 >= center.y() && y1 <= center.y()+height) {
                closest_t = t1;
                hit_anything = true;
                rec.t = t1;
                rec.p = r.point_at_parameter(t1);
                vec3 outward_normal = (rec.p - vec3(center.x(), rec.p.y(), center.z())) / radius;
                rec.normal = outward_normal;
                rec.mat_ptr = mat_ptr;
                rec.u = atan2(rec.p.z()-center.z(), rec.p.x()-center.x()) / (2*M_PI);
                rec.v = (rec.p.y()-center.y()) / height;
            }
        }
    }

    // Проверка нижней крышки (y = center.y())
    if (fabs(r.direction().y()) > 1e-6f) {
        float t = (center.y() - r.origin().y()) / r.direction().y();
        if (t > t_min && t < t_max) {
            float px = r.origin().x() + t*r.direction().x();
            float pz = r.origin().z() + t*r.direction().z();
            float dist2 = (px-center.x())*(px-center.x()) + (pz-center.z())*(pz-center.z());
            if (dist2 <= radius*radius && t < closest_t) {
                closest_t = t;
                hit_anything = true;
                rec.t = t;
                rec.p = r.point_at_parameter(t);
                rec.normal = vec3(0, -1, 0);
                rec.mat_ptr = mat_ptr;
                rec.u = (px-center.x())/(2*radius) + 0.5f;
                rec.v = (pz-center.z())/(2*radius) + 0.5f;
            }
        }
    }

    // Проверка верхней крышки (y = center.y()+height)
    float top_y = center.y() + height;
    if (fabs(r.direction().y()) > 1e-6f) {
        float t = (top_y - r.origin().y()) / r.direction().y();
        if (t > t_min && t < t_max) {
            float px = r.origin().x() + t*r.direction().x();
            float pz = r.origin().z() + t*r.direction().z();
            float dist2 = (px-center.x())*(px-center.x()) + (pz-center.z())*(pz-center.z());
            if (dist2 <= radius*radius && t < closest_t) {
                closest_t = t;
                hit_anything = true;
                rec.t = t;
                rec.p = r.point_at_parameter(t);
                rec.normal = vec3(0, 1, 0);
                rec.mat_ptr = mat_ptr;
                rec.u = (px-center.x())/(2*radius) + 0.5f;
                rec.v = (pz-center.z())/(2*radius) + 0.5f;
            }
        }
    }

    return hit_anything;
}

bool cylinder::bounding_box(float t0, float t1, aabb& box) const {
    box = aabb(center - vec3(radius, 0, radius),
               center + vec3(radius, height, radius));
    return true;
}

#endif
