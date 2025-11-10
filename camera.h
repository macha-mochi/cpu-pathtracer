//
// Created by Faye Yu on 10/29/25.
//

#ifndef CAMERA_H
#define CAMERA_H

#include "color.h"
#include "hittable.h"
#include "material.h"
#include "ray.h"
#include "rtweekend.h"

class camera
{
public:
    //Image

    double aspect_ratio = 1.0; //ratio of image width over image height
    int image_width = 100; //rendered img width in pixel count
    int samples_per_pixel = 10; //count of random samples for each pixel
    int max_depth = 10; //maximum number of ray bounces into scene

    double vfov = 90; //vertical view angle (field of view)
    point3 lookfrom = point3(0, 0, 0); //point camera is looking from
    point3 lookat = point3(0, 0, -1); //point camera is looking at
    vec3 vup = vec3(0, 1, 0); //camera-relative "up" direction. NOT the 'y' basis of the camera plane

    double defocus_angle = 0; //Variation angle of rays thru each pixel
    double focus_dist = 10; //distance from camera lookfrom point to plane of perfect focus

    void render(const hittable&world)
    {
        initialize();

        //Render
        std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << image_height - j << " " << std::flush;
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; sample++)
                {
                    ray r = get_ray(i, j);
                    pixel_color+=ray_color(r, max_depth, world); //just a vector3 so we can add
                }

                //pixel_samples_scale is what we need to mult by to average out pixel_color
                //we average it out to get anti alias
                write_color(std::cout, pixel_samples_scale * pixel_color);
            }
        }

        std::clog << "Done!\n";

    }
private:
    int image_height = 100; //rendered image height
    double pixel_samples_scale = 1.0; //Color scale factor for  a sum of pixel samples
    point3 center; // camera  center
    point3 pixel00_loc; //location of pixel 0, 0
    vec3 pixel_delta_u; //offset to pixel to the right
    vec3 pixel_delta_v; //offset to pixel below
    vec3 u, v, w; //camera frame basis vectors
    vec3 defocus_disk_u; //defocus disk horizontal radius
    vec3 defocus_disk_v; //defocus disk vertical radius

    void initialize()
    {
        //calculate image height and ensure it's at least 1
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        //Determine viewpoint dimensions
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2.0);
        auto viewport_height = 2.0*h*focus_dist;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        //calculate u, v, w unit basis vecs for camera coordinate frame
        w = unit_vector(lookfrom - lookat); //again we look in -w direction
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        //Calc the vectors across the horizontal and down the vertical viewport edges
        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;

        //Calc the hrizontal and vertica delta vectors from pixel to pixel
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        //Calc location of the  upper left pixel
        //reminder: x is right, y is up, negative z is viewing direction
        auto viewpoint_upper_left = center
                                        - focus_dist * w
                                        - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewpoint_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        //Calculate the camera defocus disk basis vectors
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle/2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }
    ray get_ray(int i, int j) const
    {
        //Construct a camera ray originating from the defocus disk
        //and directed at randomly sampled point around the pixel i, j

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);
        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin; //ray_origin + this ray = pixel_sample
        return ray(ray_origin, ray_direction);
    }
    vec3 sample_square() const
    {
        //Returns the vector to a random point in the unit square centered at origin
        return vec3(random_double() - 0.5, random_double() + 0.5, 0);
    }
    point3 defocus_disk_sample() const
    {
        //Returns a random point in the camera defocus disk
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u + p[1] * defocus_disk_v);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const
    {
        if (depth <= 0)
        {
            //weve exceeded ray bounce limit
            return color(0, 0, 0);
        }
        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec))
        {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation,scattered))
                return attenuation * ray_color(scattered, depth-1, world);
            return color(0, 0, 0);
        }
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif //CAMERA_H
