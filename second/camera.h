#ifndef CAMERA_H
#define CAMERA_H

#include "rtweekend.h"

class camera{
public:
    camera(
        point3 lookfrom,
        point3 lookat,
        vec3 vup, // 定义上与v 和 w 共面
        double vfov, // vertical field-of-view in degrees
        double aspect_ratio,  // 16.0 / 9.0
        double aperture,    // 光圈
        double focus_dist,   // 焦距
        double _time0 = 0,
        double _time1 = 0
    ){
        auto theta = degrees_to_radians(vfov);
        auto h = tan(theta/2);
        auto viewport_height = 2.0 * h;  // 2.0
        auto viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(lookfrom - lookat); // Figure 16: Camera view up direction
        u = unit_vector(cross(vup, w)); // vup v and w are in a same plane, using this feature to generate our camera
        v = cross(w, u);

        // 更大的focus_dist增大了整个viewport的窗口大小。
        // 焦距大景深小，得到的图就有更大面积模糊。(- focus_dist * w) 这里将平面推向了远离焦点的位置，超过coc容忍面积变糊。
        // 窗口大，可供选择的ray direction多，容易打到同一个像素上，颜色混叠，blur。
        // 反映在代码上，大窗口上有更多的点接受光线，而样本数不变，图变糊。
        origin = lookfrom;
        horizontal = focus_dist * viewport_width * u;  // 将viewport_width迁移到 u坐标方向上 u(0, 1) -> (0, width)
        vertical = focus_dist * viewport_height * v;   // 将viewport_height迁移到 v坐标方向上 v(0, 1) -> (0, height)
        lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist * w;  // viewport上的左下角, w方向是始终与相机平面垂直的
        
        lens_radius = aperture / 2;
        time0 = _time0;
        time1 = _time1;
    }

    ray get_ray(double s, double t) const{
        vec3 rd = lens_radius * random_in_unit_disk();
        vec3 offset = u * rd.x() + v * rd.y();

        return ray(
            origin + offset,
            lower_left_corner + s*horizontal + t*vertical - (origin + offset),
            random_double(time0, time1)         // 这里给光线分配随机的时间参数，使得hit函数中的center(time)发生变化，而产生运动球的动态模糊
        );
    }

private:
    point3 origin;
    point3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    double lens_radius;
    double time0, time1;  // shutter open/close times
};

#endif
