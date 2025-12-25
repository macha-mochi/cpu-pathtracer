//
// Created by Faye Yu on 10/30/25.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "hittable.h"
#include "bxdf.h"

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
    virtual bsdf create_bsdf(const hit_record& rec) const
    {
        return bsdf{rec};
    }
    virtual color emitted() const
    {
        return color(0, 0, 0); //default emit is black
    }
};

class lambertian : public material
{
public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override
    {
        //RT in one weekend impl
        /*auto scatter_direction = rec.normal + random_unit_vector();

        //catch degen scatter directions
        if (scatter_direction.near_zero())
        {
            scatter_direction = rec.normal;
        }
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;*/

        /*const vec3& wo = r_in.direction() * -1.0;
        bsdf_sample s = sample(wo, rec);
        scattered = ray(rec.p, s.wi);
        attenuation = albedo;*/
        return true;
    }

    bsdf create_bsdf(const hit_record& rec) const override
    {
        bsdf b = bsdf{rec};
        b.add<lambertian_reflection>(albedo);
        return b;
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
    static double Fr_conductor(double cos_theta_i, double eta_i, double eta_t, double k_t){
        /*double eta = eta_t / eta_i;
        double k = k_t / eta_i;

        double cos_squared = cos_theta_i * cos_theta_i;
        double ab_temp = eta*eta - k*k - (1-cos_squared);
        double a2_b2 = sqrt(ab_temp * ab_temp + 4 * eta * eta * k * k);

        double r_perp = a2_b2 - 2 *
        */
        return 1.0;
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

        bool refracted = false;
        if (cannot_refract || schlick_reflectance(cos_theta, ri) > random_double())
        {
            direction = reflect(unit_direction, rec.normal);
        }
        else
        {
            direction = refract(unit_direction, rec.normal, ri);
            refracted = true;
            //both unit_direction and rec.normal are unit vectors which the 'refract' method needs
        }

        scattered = ray(rec.p, direction);
        if (refracted && rec.front_face) scattered.set_eta(refraction_index);
        //only update ior if it was refract and INTO the material - if you're refracting
        //out of say, glass into air it should be reset to 1 with creation of new way
        return true;
    }
    color f_s(const vec3& wo, const vec3& wi, const hit_record& rec) const
    {
        return color(0,0,0); //nothing scatters in any direction except the one light reflects in
    }
    double pdf(const vec3& wo, const vec3& wi, const hit_record& rec) const
    {
        return 0.0; //we don't use a pdf for speculars bc there's only one correct direction we can calculate
    }
    bsdf_sample sample(const vec3& wo, const hit_record& rec) const
    {
        //assume smooth for this, can add microfacets in the gpu version hehe
        vec3 unit_direction = unit_vector(wo);
        //theta is angle between incoming ray and normal
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        cos_theta = std::abs(cos_theta);

        //need to find eta_i of medium ray is currently in, eta_t is known
        double eta_i = rec.incident_eta;
        double eta_t = rec.front_face ? refraction_index : 1.0;
        double reflectance = Fr_dielectric(cos_theta, eta_i, eta_t);
        vec3 wi;
        if (random_double() < reflectance) //reflect
        {
            wi = reflect(unit_direction, rec.normal);
            double f_s = reflectance / cos_theta;
            //assuming clear glass aka attenuation = all 1
            return bsdf_sample(wi, color(1.0, 1.0, 1.0) * f_s, 1, true);
        }else //refract
        {
            wi = refract(unit_direction, rec.normal, eta_i/eta_t);
            //TODO somehow update the ior of the ray
            double f_s = (1-reflectance) / cos_theta;
            return bsdf_sample(wi, color(1.0, 1.0, 1.0) * f_s, 1, true);

        }
    }
private:
    //refractive index in vacuum or air, or the ratio of the material's refractive index
    //over the refractive index of the enclosing media
    double refraction_index;

    static double schlick_reflectance(double cosine, double refraction_index)
    {
        //use Schlick's apprxoimation for reflectance (yeah idk what this is)
        //reflectance: how much light reflects off a surface
        auto r0 = (1-refraction_index) / (1+refraction_index);
        r0 = r0 * r0;
        return r0 + (1-r0)*std::pow((1-cosine), 5);
    }

    //eta_i : index of refraction for incident material
    //eta_t : index of refraction for transmitted material
    //returns how much light is reflected (0 = none, 1 = all)
    static double Fr_dielectric(double cos_theta_i, double eta_i, double eta_t)
    {
        if (cos_theta_i < 0)
        {
            //the ray is on the inside, swap the etas
            double temp = eta_i;
            eta_i = eta_t;
            eta_t = temp;
            cos_theta_i = std::abs(cos_theta_i); //ensure cos_theta_i is nonneg
        }

        //find cos_theta_t using snell's law
        double sin_theta_i = std::sqrt(std::max(static_cast<double>(0), 1 - cos_theta_i * cos_theta_i));
        double sin_theta_t = eta_i * sin_theta_i / eta_t;
        if (sin_theta_t >= 1)
        {
            return 1; //no solution for snell's law, all light reflected
        }
        double cos_theta_t = std::sqrt(std::max(static_cast<double>(0), 1 - sin_theta_t * sin_theta_t));

        double r_parallel = (eta_t * cos_theta_i - eta_i * cos_theta_t)
        / (eta_t * cos_theta_i + eta_i * cos_theta_t);
        double r_perp = (eta_i * cos_theta_i - eta_t * cos_theta_t)
        / (eta_i * cos_theta_i + eta_t * cos_theta_t);

        return (r_parallel * r_parallel + r_perp * r_perp) / 2;
    }
};

class diffuse_light : public lambertian
{
public:
    diffuse_light(const color& emit, const color& albedo) : emit(emit), lambertian(albedo)
    {}

    color emitted() const override
    {
        return emit;
    }
private:
    color emit;
};



#endif //MATERIAL_H
