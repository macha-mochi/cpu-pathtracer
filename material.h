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

    virtual bsdf create_bsdf(const hit_record& rec) const
    {
        return bsdf{rec};
    }
    virtual color emitted() const
    {
        return color(0, 0, 0); //default emit is black
    }
    virtual std::string to_string() const = 0;
};

class lambertian : public material
{
public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bsdf create_bsdf(const hit_record& rec) const override
    {
        bsdf b = bsdf{rec};
        b.add<lambertian_reflection>(albedo);
        return b;
    }
    std::string to_string() const override
    {
        return "Lambertian with albedo " + albedo.to_string();
    }
private:
    color albedo;
};

struct complex_ior
{
    double eta;
    double k;
};

namespace Metal
{
    static constexpr complex_ior steel{2.485, 3.433};
    static constexpr complex_ior silver{0.177, 3.638};
    static constexpr complex_ior gold{0.37, 2.82};
    static constexpr complex_ior copper{0.617, 2.63};
}

class metal : public material
{
public:
    explicit metal(const complex_ior& i) : eta_i(1.0), eta_t(i.eta), k_t(i.k),
    f(1.0, eta_t, k_t), albedo(color(1, 1, 1)), fuzz(1) {}
    metal(const complex_ior& i, const color& albedo) : eta_i(1.0), eta_t(i.eta), k_t(i.k),
    f(1.0, eta_t, k_t), albedo(albedo), fuzz(1) {}
    metal(const double eta_t, const double k_t, const color& albedo, double fuzz) :
    eta_i(1.0), eta_t(eta_t), k_t(k_t), f(eta_i, eta_t, k_t), albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1)
    {}
    metal(const double eta_i, const double eta_t, const double k_t, const color& albedo, double fuzz) :
    eta_i(eta_i), eta_t(eta_t), k_t(k_t), f(eta_i, eta_t, k_t), albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1)
    {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const
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
    bsdf create_bsdf(const hit_record& rec) const override
    {
        bsdf b = bsdf{rec};
        b.add<specular_reflection>(albedo, &f);
        return b;
    }
    std::string to_string() const override
    {
        return "Metal with albedo " + albedo.to_string();
    }

private:
    double eta_i, eta_t, k_t;
    color albedo;
    double fuzz;
    const fresnel_conductor f;
};

class dielectric : public material
{
public:
    dielectric(double eta) : reflection_color(1, 1, 1), transmission_color(1, 1, 1), eta_a(1.0), eta_b(eta){}
    dielectric(double eta_a, double eta_b) : reflection_color(1, 1, 1), transmission_color(1, 1, 1),
    eta_a(eta_a), eta_b(eta_b){}
    dielectric(const color& r, const color& t, double eta_a, double eta_b) :
    reflection_color(r), transmission_color(t), eta_a(eta_a), eta_b(eta_b) {}

    std::string to_string() const override
    {
        return "Dielectric with R color " + reflection_color.to_string() +
            ", T color " + transmission_color.to_string() +
                ", with eta = " + std::to_string(eta_b) + " in medium with eta = " + std::to_string(eta_a);
    }

    bsdf create_bsdf(const hit_record& rec) const override
    {
        bsdf b = bsdf{rec};
        b.add<fresnel_specular>(reflection_color, transmission_color, eta_a, eta_b);
        return b;
    }
private:
    //refractive index in vacuum or air, or the ratio of the material's refractive index
    //over the refractive index of the enclosing media
    color reflection_color;
    color transmission_color;
    double eta_a, eta_b; //above = the side the surface normal is in/enclosing medium, below = the other side/entered medium

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
