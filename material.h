//
// Created by Faye Yu on 10/30/25.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "hittable.h"

class material
{
public:
    virtual ~material() = default;

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const
    {
        return false;
    }
};

class lambertian : public material
{
public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override
    {
        auto scatter_direction = rec.normal + random_unit_vector();

        //catch degen scatter directions
        if (scatter_direction.near_zero())
        {
            scatter_direction = rec.normal;
        }
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        //attenuation is just what % of light is reflected in each channel which depends on the color of the obj
        //since the channels that with higher % reflected are those corresponding to the obj color
        return true;
        //instead of returning multiple vars we simply change the non-cons references ohhhhh
    }
private:
    color albedo;
};

class metal : public material
{
public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1){}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override
    {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        // normalize the 'reflected' vector and add a random vector on unit sphere
        // to make the reflection not as perfect (to add fuzz)
        reflected = unit_vector(reflected) + fuzz * random_unit_vector();
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        //if ray 'scattered' is pointed inside surface then discard
        return (dot(scattered.direction(), rec.normal) > 0);
    }
private:
    color albedo;
    double fuzz;
};

class dielectric : public material
{
public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override
    {
        attenuation = color(1.0, 1.0, 1.0); //absorb no light
        //if you hit a front face you're entering the material so
        //multiply by IOR, else you're exiting so divide by IOR
        double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        //theta is angle between incoming ray and normal
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = ri*sin_theta > 1.0; //no solution for snell's law
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);
        //both unit_direction and rec.normal are unit vectors which the 'refract' method needs

        scattered = ray(rec.p, direction);
        return true;
    }
private:
    //refractive index in vacuum or air, or the ratio of the material's refractive index
    //over the refractive index of the enclosing media
    double refraction_index;

    static double reflectance(double cosine, double refraction_index)
    {
        //use Schlick's apprxoimation for reflectance (yeah idk what this is)
        //reflectance: how much light reflects off a surface
        auto r0 = (1-refraction_index) / (1+refraction_index);
        r0 = r0 * r0;
        return r0 + (1-r0)*std::pow((1-cosine), 5);
    }
};


#endif //MATERIAL_H
