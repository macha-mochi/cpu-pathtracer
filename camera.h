//
// Created by Faye Yu on 10/29/25.
//

#ifndef CAMERA_H
#define CAMERA_H

#include <omp.h>
#include "color.h"
#include "hittable.h"
#include "light.h"
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
    color background; //scene background color;

    double vfov = 90; //vertical view angle (field of view)
    point3 lookfrom = point3(0, 0, 0); //point camera is looking from
    point3 lookat = point3(0, 0, -1); //point camera is looking at
    vec3 vup = vec3(0, 1, 0); //camera-relative "up" direction. NOT the 'y' basis of the camera plane
    bool flipHorizontal = false; //FLIPHORIZONTAL SHOULD BE TRUE IF UR LOOKING IN +W DIRECTION INSTEAD OF -W

    double defocus_angle = 0; //Variation angle of rays thru each pixel
    double focus_dist = 10; //distance from camera lookfrom point to plane of perfect focus

    bool russian_roulette_termination = false; //whether the render loop will use rr

    void render(const hittable& world)
    {
        hittable_list lights;
        render(world, lights);
    }
    void render(const hittable& world, const hittable_list& lights)
    {
        initialize();

        std::vector<color> image_buffer(image_width * image_height);

        int rows_done = 0;
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << image_height - rows_done << " " << std::flush;
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; sample++)
                {
                    ray r = get_ray(i, j);
                    //throughput = color(1, 1, 1); //need to reset the throughput before each ray_color (RECURSIVE) call
                    color c = ray_color_iter(r, max_depth, world, lights);
                    pixel_color+=c;
                }

                //pixel_samples_scale is what we need to mult by to average out pixel_color
                //we average it out for monte carlo and anti alias
                int index = j * image_width + i;
                image_buffer[index] = pixel_samples_scale * pixel_color;
            }
            rows_done++;
        }

        //write to image
        std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";
        for(const color& c : image_buffer)
        {
            write_color(std::cout, c);
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
    interval russian_roulette_clamp;
    color throughput;

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
        if (flipHorizontal) viewport_u = -viewport_u; //FLIPHORIZONTAL SHOULD BE TRUE IF UR LOOKING IN +W DIRECTION INSTEAD
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

        russian_roulette_clamp = interval(0.05, 1);
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

    color ray_color_iter(const ray& camera_ray, int depth, const hittable& world, const hittable_list& lights)
    {
        std::string debug = "";
        color throughput = color(1, 1, 1);
        color outgoing_radiance = color(0, 0, 0);
        int num_lights = lights.objects.size();

        ray r = camera_ray;
        color f_s;
        double cos_theta_i;
        double pdf_b;
        bool hit_specular = false;

        bool glancing_metal = false;
        bool first_hit_metal = false;

        for (int i = 0; i < depth; i++)
        {
            if (russian_roulette_termination && i >= 4)
            {
                double survive_p = std::max(throughput.x(), std::max(throughput.y(), throughput.z()));
                survive_p = russian_roulette_clamp.clamp(survive_p);
                if (random_double(0, 1) <= survive_p) //survive
                {
                    throughput *= 1 / survive_p;
                }else break;
            }

            hit_record rec;
            bool hit_anything = world.hit(r, interval(0.001, infinity), rec);

            if (i != 0) //not camera ray and we hit something, update throughput
            {
                double w_bsdf = 1;
                if (!hit_specular && hit_anything && rec.hit_light && rec.front_face)
                {
                    double pb_bsdf = pdf_b;
                    double pb_light = rec.light_source->pdf(r.origin(), rec.p);
                    pb_light*=1.0/num_lights;
                    w_bsdf = power_heuristic(pb_bsdf, pb_light);
                } //if didn't hit a light on the emitting side, w_bsdf stays at 1
                if (f_s.length_squared() == 0 || pdf_b == 0) break; //no more contributions
                throughput = throughput * w_bsdf * (f_s * cos_theta_i / pdf_b);

                debug+= "CALCULATING THROUGHPUT: f: " + f_s.to_string() + " cos: " + std::to_string(cos_theta_i) + " pdf: " + std::to_string(pdf_b) + "\n";
                debug+="w_bsdf: " + std::to_string(w_bsdf) + " change in throughput = " + (w_bsdf * (f_s * cos_theta_i / pdf_b)).to_string() + "\n";
                debug+="THROUGHPUT IS " + throughput.to_string() + "\n";
            }

            if (!hit_anything)
            {
                outgoing_radiance += (throughput * background);
                break;
            }

            //Le term
            color color_from_emission = rec.front_face ? rec.mat->emitted() : color(0, 0, 0);
            outgoing_radiance += throughput * color_from_emission;
            debug+="hit material: " + rec.mat->to_string() + " front: " + std::to_string(rec.front_face) + "\n";
            /*if (color_from_emission.length_squared() > 1e-8)
            {
                debug+="EMISSION CONTRIBUTION: added " + (throughput * color_from_emission).to_string() + "\n";
            }*/

            //sample BSDF for this shading point (rec.p)
            bsdf b = rec.mat->create_bsdf(rec, r);
            vec3 wo = unit_vector(-r.direction());
            bsdf_sample b_sample = b.sample(wo);
            vec3 wi = b_sample.wi; //is in the same hemisphere as the normal
            f_s = b_sample.f;
            cos_theta_i = std::abs(dot(wi, rec.normal)); //bc the cos(theta) just represents a ratio of areas
            if (rec.mat->get_type() == Metallic && cos_theta_i < 0.15f) glancing_metal = true;
            if (rec.mat->get_type() == Metallic && i == 0) first_hit_metal = true;
            pdf_b = b_sample.pdf;
            r = ray(rec.p, wi);
            hit_specular = b_sample.is_delta;

            //sample a direct light source via NEE
            if (num_lights == 0 || hit_specular) continue; //if no lights, or delta, dont nee
            int light_index = random_int(0, num_lights - 1);
            auto& chosen_light_ptr = lights.objects[light_index];
            auto* chosen_light = dynamic_cast<light*>(chosen_light_ptr.get());
            light_sample l_sample = chosen_light->sample(rec.p);
            color direct_color;
            if (l_sample.p_solid_angle <= 0) continue; //no nee contribution since probabilistically impossible
            ray shadow_ray = ray(rec.p, l_sample.wi);
            hit_record chosen_light_rec;
            chosen_light->hit(shadow_ray, interval(0.001, infinity), chosen_light_rec);
            hit_record world_shadow_rec;
            bool occluded = world.hit(shadow_ray, interval(0.001, chosen_light_rec.t - 1e-8), world_shadow_rec);
            if (occluded) continue; //no nee contribution since the ray hit smth before it could reach the light source
            double pdf_l = l_sample.p_solid_angle * 1.0/num_lights;
            //TODO this is just uniform random picking of lights possibly change later
            color f_s_l = b.f_s(wo, l_sample.wi);
            double cos_theta_l = std::abs(dot(rec.normal, l_sample.wi)); //another factor from rendering eq
            //TODO if you ever have transparent stuff that uses NEE will have to change this to absolute as well and make the shadow ray actually traced through refracts and stuff... this is why we want bdpt
            direct_color = f_s_l * cos_theta_l * l_sample.emitted / pdf_l;

            //calculate NEE weight //TODO for point or directional lights, expected contribution for w_light is only from nee since prob that bsdf hits that exact dir is 0
            double pl_light = l_sample.p_solid_angle * 1.0/num_lights;
            double pl_bsdf = b.pdf(wo, l_sample.wi);
            double w_light = power_heuristic(pl_light, pl_bsdf);
            outgoing_radiance += throughput * w_light * direct_color;
            debug+="NEE CONTRIBUTION: w_light = " + std::to_string(w_light) + " added: " + (throughput * w_light * direct_color).to_string() + "\n";
        }
        debug+="RETURNING FINAL COLOR: " + outgoing_radiance.to_string() + "\n";
        if (first_hit_metal && glancing_metal)
        {
            //std::clog << debug << std::endl;
        }
        return outgoing_radiance;
    }

    color ray_color(const ray& r, int depth, const hittable& world, const hittable_list& lights)
    {
        if (depth <= 0)
        {
            //weve exceeded ray bounce limit
            return color(0, 0, 0);
        }
        hit_record rec;

        //if ray hits nothing, return background color
        if (!world.hit(r, interval(0.001, infinity), rec))
        {
            return background;
        }

        int num_lights = lights.objects.size();

        //BSDF sampling
        bsdf b = rec.mat->create_bsdf(rec, r);
        vec3 wo = unit_vector(-r.direction());
        bsdf_sample b_sample = b.sample(wo);
        vec3 wi = b_sample.wi; //is in the same hemisphere as the normal
        double cos_theta_x = dot(wi, rec.normal);
        ray scattered = ray(rec.p, wi);
        color color_from_emission = rec.front_face ? rec.mat->emitted() : color(0, 0, 0);
        color throughput_change = (b_sample.f * cos_theta_x / b_sample.pdf);
        throughput = throughput * throughput_change;
        color indirect_color;
        color next;
        if (russian_roulette_termination && max_depth - depth >= 4)
        {
            double survive_p = std::max(throughput.x(), std::max(throughput.y(), throughput.z()));
            survive_p = russian_roulette_clamp.clamp(survive_p);
            if (random_double(0, 1) <= survive_p) //survive
            {
                next = ray_color(scattered, depth - 1, world, lights);
                indirect_color = throughput_change * next / survive_p;
            }else
            {
                indirect_color = color(0, 0, 0);
            }
        }else
        {
            next = ray_color(scattered, depth - 1, world, lights);
            indirect_color = throughput_change * next;
        }

        if (b_sample.is_delta)
        {
            //don't nee since it's delta, use bsdf only
            return color_from_emission + indirect_color;
        }
        //calculate bsdf weight in mis
        hit_record any_light_rec;
        bool hit_light = lights.hit(scattered, interval(0.001, infinity), any_light_rec);
        double pb_light = 0;
        if (hit_light && any_light_rec.front_face)
        {
            hit_record world_shadow_rec;
            bool occluded = world.hit(scattered, interval(0.001, any_light_rec.t - 1e-8), world_shadow_rec);
            if (!occluded)
            {
                //p_light is nonzero
                double temp = any_light_rec.light_source->pdf(rec.p, any_light_rec.p);
                pb_light = temp * 1.0/num_lights;
            }
        }
        double pb_bsdf = b_sample.pdf;
        double w_bsdf = power_heuristic(pb_bsdf, pb_light);

        //NEE sampling
        auto& chosen_light_ptr = lights.objects[random_int(0, num_lights - 1)];
        auto* chosen_light = dynamic_cast<light*>(chosen_light_ptr.get());
        light_sample l_sample = chosen_light->sample(rec.p);
        color direct_color;
        if (l_sample.p_solid_angle > 0)
        {
            ray shadow_ray = ray(rec.p, l_sample.wi);
            hit_record chosen_light_rec;
            chosen_light->hit(shadow_ray, interval(0.001, infinity), chosen_light_rec);
            hit_record world_shadow_rec;
            bool occluded = world.hit(shadow_ray, interval(0.001, chosen_light_rec.t - 1e-8), world_shadow_rec);
            if (occluded)
            {
                //the ray hit something before it hit the light rec, so is occluded, no nee
                direct_color = color(0, 0, 0);
            }else
            {
                double pdf = l_sample.p_solid_angle * 1.0/num_lights;
                //TODO this is just uniform random picking of lights possibly change later
                color f_s_l = b.f_s(wo, l_sample.wi);
                double cos_theta_l = dot(rec.normal, l_sample.wi);
                direct_color = f_s_l * cos_theta_l * l_sample.emitted / pdf;
            }
        }
        if (direct_color.near_zero())
        {
            //means either p_solid_angle is 0 or the ray was occluded, either way don't do nee
            return color_from_emission + w_bsdf * indirect_color;
        }
        //calculate NEE weight //TODO for point or directional lights, expected contribution for w_light is only from nee since prob that bsdf hits that exact dir is 0
        double pl_light = l_sample.p_solid_angle * 1.0/num_lights;
        double pl_bsdf = b.pdf(wo, l_sample.wi);
        double w_light = power_heuristic(pl_light, pl_bsdf);

        color result = color_from_emission + w_bsdf * indirect_color + w_light * direct_color;
        return result;
    }
    //in the format of (p1)^2 / ((p1)^2 + (p2)^2)
    static double power_heuristic(double p1, double p2)
    {
        return p1 * p1 / (p1 * p1 + p2 * p2);
    }
};

#endif //CAMERA_H
