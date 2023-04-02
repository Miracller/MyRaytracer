#include "rtweekend.h"

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "moving_sphere.h"
#include "bvh.h"
#include "aarect.h"
#include "box.h"

#include <iostream>
// #include <mutex>
#include <thread>

// std::mutex mtx;

color ray_color(const ray& r, const color& background, const hittable& world, int depth){

    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if(depth <= 0){
        return color(0, 0, 0);
    }

    // If the ray hits nothing, return the background color.
    if(!world.hit(r, 0.001, infinity, rec)){
        return background;
    }

    ray scattered;
    color attenuation;
    color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
    double pdf;
    color albedo;

    if(!rec.mat_ptr->scatter(r, rec, albedo, scattered, pdf)){
        return emitted;
    }

    auto on_light = point3(random_double(213, 343), 554, random_double(227, 332));
    auto to_light = on_light - rec.p;
    auto distance_squared = to_light.length_squared();
    to_light = unit_vector(to_light);
    if(dot(to_light, rec.normal) < 0){
        return emitted;
    }

    double light_area = (343-213)*(332-227);
    // 这里其实是 to_light向量 乘以（0,1,0），求的是cos(n_y, to_light)，因为光源法线方向固定为（0,1,0）
    auto light_cosine = fabs(to_light.y());
    if(light_cosine < 0.000001){
        return emitted;
    }

    pdf = distance_squared / (light_cosine * light_area);
    scattered = ray(rec.p, to_light, r.time());

    // scatter color = A * s(direction) * color(direction) / p(direction)
    // s(direction): a directional distribution over solid angles
    return emitted 
        + albedo * rec.mat_ptr->scattering_pdf(r, rec, scattered)
                 * ray_color(scattered, background, world, depth-1) / pdf;
}

hittable_list cornell_box(){
    hittable_list world, objects;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    world.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    world.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
    // world.add(make_shared<xz_rect>(213, 343, 227, 332, 554, light));
    world.add(make_shared<flip_face>(make_shared<xz_rect>(213, 343, 227, 332, 554, light)));
    world.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    world.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    world.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));
    // world.add(make_shared<box>(point3(130, 0, 65), point3(295, 165, 230), white));
    // world.add(make_shared<box>(point3(265, 0, 295), point3(430, 330, 460), white));

    shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265, 0, 295));
    world.add(box1);

    shared_ptr<hittable> box2 = make_shared<box>(point3(0, 0, 0), point3(165, 165, 165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));
    world.add(box2);

    objects.add(make_shared<bvh_node>(world, 0, 1));
    return objects;
}

int main(){
    // Image
    const auto aspect_ratio = 1.0 / 1.0;
    const int image_width = 300;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 10;
    const int max_depth = 50;

    // World
    auto world = cornell_box();
    // hittable_list world;
    color background(0,0,0);

    // Camera
    // point3 lookfrom(13,2,3);
    // point3 lookat(0,0,0);
    point3 lookfrom(278, 278, -800);
    point3 lookat(278, 278, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;
    auto vfov = 40.0;
    auto time0 = 0.0;
    auto time1 = 1.0;

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    // Render
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    // 多线程，分行
    const int th_num = 20;
    int times = image_height / th_num;
    std::thread th[th_num];
    std::vector<color> framebuffer(image_height * image_width, color(0, 0, 0));

    auto castRayMultiThread = [&](int y_min, int y_max){
        for (int j = y_min; j < y_max; ++j) {
            
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                for(int s = 0; s < samples_per_pixel; ++s){
                    auto u = (i + random_double()) / (image_width-1);
                    auto v = (j + random_double()) / (image_height-1);
                    ray r = cam.get_ray(u, v);
                    pixel_color += ray_color(r, background, world, max_depth);
                }
                framebuffer[j * image_width + i] = pixel_color;
            }
        }
    };

    for(int i=0; i < th_num; ++i){
        std::cerr << i << std::flush;
        th[i] = std::thread(castRayMultiThread, i * times, (i+1) * times);
    }

    for(int i=0; i < th_num; ++i){
        th[i].join();
    }

    // 不用多线程的版本 ----------------------------
    // from left to right. (0, 0) 在右上角
    // for (int j = image_height-1; j >= 0; --j) {
    //     std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
    //     for (int i = 0; i < image_width; ++i) {
    //         color pixel_color(0, 0, 0);
    //         for(int s = 0; s < samples_per_pixel; ++s){
    //             auto u = (i + random_double()) / (image_width-1);
    //             auto v = (j + random_double()) / (image_height-1);
    //             ray r = cam.get_ray(u, v);
    //             pixel_color += ray_color(r, background, world, max_depth);
    //         }
    //         write_color(std::cout, pixel_color, samples_per_pixel);
    //     }
    // }
    // ----------------------------
    for (int j = image_height-1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            // std::cerr << framebuffer[j * image_width + i] << std::flush;
            write_color(std::cout, framebuffer[j * image_width + i], samples_per_pixel);
        }
    }

    std::cerr << "\nDone.\n";
    return 0;
}
