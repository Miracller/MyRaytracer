#ifndef MATERIAL_H
#define MATERIAL_H

#include "rtweekend.h"
#include "hittable.h"
#include "texture.h"
#include "onb.h"

// struct hit_record;

class material {
public:
    virtual color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const {
        return color(0,0,0);
    }
    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& albedo, ray& scattered, double& pdf
    ) const{
        return false;
    }

    virtual double scattering_pdf(
        const ray& r_in, const hit_record& rec, const ray& scattered
    ) const {
        return 0;
    }
};

class lambertian : public material{
public:
    lambertian(const color& a) : albedo(make_shared<solid_color>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}
    
    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& alb, ray& scattered, double& pdf
    )const override{
        // auto scatter_direction = rec.normal + random_unit_vector();

        // // Catch degenerate scatter direction
        // if(scatter_direction.near_zero()){
        //     scatter_direction = rec.normal;
        // }
        // scattered = ray(rec.p, unit_vector(scatter_direction), r_in.time());

        // 生成随机正交基
        onb uvw;
        uvw.build_from_w(rec.normal);
        auto direction = uvw.local(random_cosine_direction());
        // auto direction = random_in_hemisphere(rec.normal);
        scattered = ray(rec.p, unit_vector(direction), r_in.time());
        
        alb = albedo->value(rec.u, rec.v, rec.p);
        // pdf = dot(rec.normal, scattered.direction()) / pi;
        // pdf = 0.5 / pi; // 半球均匀采样

        // p(direction) = cos(theta) / pi
        pdf = dot(uvw.w(), scattered.direction()) / pi;
        return true;
    }

    // scattring pdf = cos(n, scatter) / pi
    double scattering_pdf(
        const ray& r_in, const hit_record& rec, const ray& scattered
    ) const {
        auto cosine = dot(rec.normal, unit_vector(scattered.direction()));
        return cosine < 0 ? 0 : cosine/pi;
    }

public:
    // diffuse light, 能量守恒情况下，(brdf) f_r = albedo / pi. 
    // albedo : [0, 1], f_r : [0, 1/pi]
    // 这里是直接当衰减用
    // third里albedo是描述光线被反射的比率（另一部分被吸收），与games101 统一
    shared_ptr<texture> albedo;
};

// class metal : public material{
// public:
//     metal(const color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

//     virtual bool scatter(
//         const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
//     ) const override{
//         vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
//         scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere(), r_in.time());
//         attenuation = albedo;
//         return (dot(scattered.direction(), rec.normal) > 0);
//     }

// public:
//     color albedo;
//     double fuzz;
// };

// class dielectric : public material{
// public:
//     dielectric(double index_of_refraction) : ir(index_of_refraction) {}

//     virtual bool scatter(
//         const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
//     ) const override{
//         attenuation = color(1.0, 1.0, 1.0); // glass absorbs nothing
//         double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

//         vec3 unit_direction = unit_vector(r_in.direction());
//         double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
//         double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

//         // 判断有没有打破snell's law 见book1 10.3，判断是否折射
//         bool cannot_refract = refraction_ratio * sin_theta > 1.0;
//         vec3 direction;

//         if(cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double()){
//             direction = reflect(unit_direction, rec.normal);
//         }else{
//             direction = refract(unit_direction, rec.normal, refraction_ratio);
//         }

//         scattered = ray(rec.p, direction, r_in.time());
//         return true;
//     }

// public:
//     double ir; // Index of Refraction // etat_over_etai

// private:
//     static double reflectance(double cosine, double ref_idx){
//         // Use Schlick's approximation for reflectance.
//         auto r0 = (1-ref_idx) / (1+ref_idx);
//         r0 = r0 * r0;
//         return r0 + (1-r0) * pow((1-cosine), 5);
//     }
// };

class diffuse_light : public material{
public:
    diffuse_light(shared_ptr<texture> a) : emit(a) {}
    diffuse_light(color c) : emit(make_shared<solid_color>(c)) {}

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf
    ) const override{
        return false;
    }

    virtual color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p) const override{
        if(rec.front_face)
            return emit->value(u, v, p);
        else
            return color(0, 0, 0);
    }

public:
    shared_ptr<texture> emit;
};


#endif
