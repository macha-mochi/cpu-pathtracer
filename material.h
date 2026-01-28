//
// Created by Faye Yu on 10/30/25.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "spectrum.h"
#include "bxdf.h"
enum material_type
{
    Lambertian, Metallic, Dielectric
};
class material
{
public:
    virtual ~material() = default;

    virtual bsdf create_bsdf(const hit_record& rec, const ray& r) const
    {
        return bsdf{rec};
    }
    virtual color emitted() const
    {
        return color(0, 0, 0); //default emit is black
    }
    virtual std::string to_string() const = 0;
    virtual material_type get_type() const = 0;
};

class lambertian : public material
{
public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bsdf create_bsdf(const hit_record& rec, const ray& r) const override
    {
        bsdf b = bsdf{rec};
        b.add<lambertian_reflection>(albedo);
        return b;
    }
    std::string to_string() const override
    {
        return "Lambertian with albedo " + albedo.to_string();
    }
    material_type get_type() const override {return type;}
private:
    color albedo;
    const material_type type = Lambertian;
};

class metal : public material
{
public:
    explicit metal(const complex_ior& i) : eta_i(1.0), f(1.0, i), albedo(color(1, 1, 1)),
    roughness(0), mf_dist(0, 0) {}
    metal(const complex_ior& i, const color& albedo) : eta_i(1.0), f(1.0, i), albedo(albedo),
    roughness(0), mf_dist(0, 0) {}
    metal(const complex_ior& i, const color& albedo, double r, double a) :
    eta_i(1.0), f(1.0, i), albedo(albedo), roughness(r < 1 ? r : 1), anisotropy(a < 1 ? a : 1),
    mf_dist(get_alpha_x(), get_alpha_y()) {}
    metal(const double eta_i, const complex_ior& i, const color& albedo, double r, double a) :
    eta_i(eta_i), f(eta_i, i), albedo(albedo), roughness(r < 1 ? r : 1), anisotropy(a < 1 ? a : 1),
    mf_dist(get_alpha_x(), get_alpha_y()) {}

    bsdf create_bsdf(const hit_record& rec, const ray& r) const override
    {
        bsdf b = bsdf{rec};
        b.add<conductor>(mf_dist, albedo, f);
        return b;
    }
    std::string to_string() const override
    {
        return "Metal with albedo " + albedo.to_string();
    }
    material_type get_type() const override {return type;}

private:
    double eta_i, eta_t, k_t;
    color albedo;
    double roughness;
    double anisotropy;
    const fresnel_conductor f;
    const trowbridge_reitz_distribution mf_dist;
    const material_type type = Metallic;

    float get_alpha_x()
    {
        return roughness * roughness / std::sqrt(1 - 0.9 * anisotropy);
    }
    float get_alpha_y()
    {
        return roughness * roughness * std::sqrt(1 - 0.9 * anisotropy);
    }
};

class dielectric : public material
{
public:
    dielectric(double eta, double r) : reflection_color(1, 1, 1), transmission_color(1, 1, 1),
    eta_a(1.0), eta_b(eta), roughness(r < 1 ? r : 1), mf_dist(roughness * roughness, roughness * roughness){}
    dielectric(double eta_a, double eta_b, double r) : reflection_color(1, 1, 1), transmission_color(1, 1, 1),
    eta_a(eta_a), eta_b(eta_b), roughness(r < 1 ? r : 1), mf_dist(roughness * roughness, roughness * roughness){}
    dielectric(const color& r, const color& t, double eta_a, double eta_b, double rough) :
    reflection_color(r), transmission_color(t), eta_a(eta_a), eta_b(eta_b),
    roughness(rough < 1 ? rough : 1), mf_dist(roughness * roughness, roughness * roughness) {}

    std::string to_string() const override
    {
        return "Dielectric with R color " + reflection_color.to_string() +
            ", T color " + transmission_color.to_string() +
                ", with eta = " + std::to_string(eta_b) + " in medium with eta = " + std::to_string(eta_a);
    }

    bsdf create_bsdf(const hit_record& rec, const ray& r) const override
    {
        bsdf b = bsdf{rec};
        vec3 wo = unit_vector(-r.direction());
        if (dot(wo, rec.outward_normal()) < 0) //not on the same side
        {
            if (mf_dist.act_as_smooth())
            {
                b.add<fresnel_specular>(reflection_color, transmission_color, eta_b, eta_a);
            }else
            {
                b.add<rough_dielectric>(reflection_color, transmission_color, eta_b, eta_a, mf_dist);
            }
        }else
        {
            if (mf_dist.act_as_smooth())
            {
                b.add<fresnel_specular>(reflection_color, transmission_color, eta_a, eta_b);
            }else
            {
                b.add<rough_dielectric>(reflection_color, transmission_color, eta_a, eta_b, mf_dist);
            }
        }
        return b;
    }
    material_type get_type() const override {return type;}
private:
    //refractive index in vacuum or air, or the ratio of the material's refractive index
    //over the refractive index of the enclosing media
    color reflection_color;
    color transmission_color;
    double eta_a, eta_b; //above = the side the surface normal is in/enclosing medium, below = the other side/entered medium
    double roughness;
    const material_type type = Dielectric;
    const trowbridge_reitz_distribution mf_dist;

    static double schlick_reflectance(double cosine, double refraction_index)
    {
        //use Schlick's apprxoimation for reflectance (yeah idk what this is)
        //reflectance: how much light reflects off a surface
        auto r0 = (1-refraction_index) / (1+refraction_index);
        r0 = r0 * r0;
        return r0 + (1-r0)*std::pow((1-cosine), 5);
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

    std::string to_string() const override
    {
        return "Diffuse light";
    }
private:
    color emit;
};



#endif //MATERIAL_H
