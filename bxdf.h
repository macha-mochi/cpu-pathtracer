//
// Created by Faye Yu on 12/19/25.
//

#ifndef BXDF_H
#define BXDF_H
#include "fresnel.h"

enum bxdf_flags
{
    Unset = 0,
    Reflection = 1 << 0,
    Transmission = 1 << 1,
    Diffuse = 1 << 2,
    Glossy = 1 << 3,
    Specular = 1 << 4,
    DiffuseReflection = Reflection | Diffuse,
    DiffuseTransmission = Transmission | Diffuse,
    GlossyReflection = Reflection | Glossy,
    GlossyTransmission = Transmission | Glossy,
    SpecularReflection = Reflection | Specular,
    SpecularTransmission = Transmission | Specular,
    All = Diffuse | Glossy | Specular | Reflection | Transmission
};
inline bool is_reflective(bxdf_flags f)
{
    return f & Reflection;
}
inline bool is_transmission(bxdf_flags f)
{
    return f & Transmission;
}
inline bool is_diffuse(bxdf_flags f)
{
    return f & Diffuse;
}
inline bool is_glossy(bxdf_flags f)
{
    return f & Glossy;
}
inline bool is_specular(bxdf_flags f)
{
    return f & Specular;
}

class bsdf_sample{
public:
    vec3 wi;
    color f;
    double pdf = 0;
    bool is_delta = false;
    bsdf_sample() = default;
    bsdf_sample(const vec3& wi, const color& f, const double pdf, const bool is_delta) :
    wi(wi), f(f), pdf(pdf), is_delta(is_delta){};
};

class bxdf
{
public:
    bxdf_flags flags = Unset;
    double magnitude = 1.0;

    virtual ~bxdf() = default;

    vec3 n = vec3(0, 0, 1);
    vec3 s = vec3(1, 0, 0);
    vec3 t = vec3(0, 1, 0);

    //all these methods assume the vectors are already in the bxdf coordinate system
    //assume that vectors passed into the bxdf methods are alr in this coord sys
    virtual color f_s(const vec3& wo, const vec3& wi) const
    {
        return color(0,0,0);
    }
    virtual double pdf(const vec3& wo, const vec3& wi) const
    {
        return 0.0;
    }
    virtual bsdf_sample sample(const vec3& wo) const
    {
        return bsdf_sample();
    }
};

class lambertian_reflection : public bxdf
{
public:
    lambertian_reflection(const color& albedo) : albedo(albedo)
    {
        flags = DiffuseReflection;
    }
    color f_s(const vec3& wo, const vec3& wi) const override
    {
        if (dot(n, wi) <= 0.0)
        {
            return color(0,0,0); //incoming ray is below the surface
        }
        return albedo / M_PI;
    }
    double pdf(const vec3& wo, const vec3& wi) const override
    {
        double cos_theta = dot(n, wi);
        if (cos_theta <= 0.0)
        {
            return 0; //prob density of getting that outgoing direction is 0
        }
        return cos_theta / M_PI;
    }
    bsdf_sample sample(const vec3& wo) const override
    {
        //generate a wi using cosine weighted hemisphere sampling
        vec3 wi = cos_weighted_random_in_hemisphere();

        //populate a bxdf_sample with calculated wi, f, pdf, is_delta
        return bsdf_sample(wi, f_s(wo, wi), pdf(wo, wi), false);
    }
private:
    color albedo;
};

class specular_reflection : bxdf
{
public:
    specular_reflection(const color& specular_color, fresnel *f) : specular_color(specular_color), f(f)
    {
        flags = SpecularReflection;
    }
private:
    color specular_color;
    fresnel *f; //ask chat what * means
};

class bsdf
{
public:
    std::vector<bxdf> bxdfs;
    const hit_record& rec;

    bsdf(const hit_record& rec) : rec(rec) {};
    void add(const bxdf& f)
    {
        bxdfs.push_back(f);
    }
    //the physically correct f_s from each bxdf
    color f_s(const vec3& wo, const vec3& wi) const
    {
        //NOTE TO SELF: might have a problem with reflection/transmission if u need to filter out certain non-delta lobes
        //or if its in the wrong hemisphere or smth
        color result = color(0, 0, 0);
        for (const auto & bxdf : bxdfs)
        {
            result+=bxdf.f_s(wo, wi); //if is delta, this will be 0 so issok
        }
        return result;
    }
    //the marginal pdf for wi, equal to sum(i = 1 -> k) Pr(choosing kth lobe) * Pr(getting wi from the kth lobe)
    double pdf(const vec3& wo, const vec3& wi) const
    {
        double w_k = 1.0/static_cast<double>(bxdfs.size() - 1); //TODO change to not be uniform later
        double result = 0.0;
        for (const auto & bxdf : bxdfs)
        {
            result+=w_k * bxdf.pdf(wo, wi); //if is delta, this will be 0 so issok
        }
        return result;
    }
    //wo is in world space
    bsdf_sample sample(const vec3& wo_world) const
    {
        //TODO i will use uniform for now and then switch to balance heuristic or whatever later so i can compare
        bxdf b = bxdfs[random_int(0, static_cast<int>(bxdfs.size()) - 1)];
        vec3 wo = local_to_render(wo_world);
        bsdf_sample sample_for_dir = b.sample(wo);
        vec3 wi = sample_for_dir.wi;
        if (sample_for_dir.is_delta)
        {
            return sample_for_dir; //if it's a delta distribution don't add up any other bxdfs into f and pdf
        }
        vec3 wi_world = render_to_local(wi);
        return bsdf_sample(wi_world, f_s(wo, wi), pdf(wo, wi), false);
    }
    vec3 local_to_render(const vec3& v_local) const
    {
        //create an orthonormal basis at the hit_record point, with up = normal
        const vec3& n = rec.normal;
        vec3 a = (std::abs(rec.normal.x()) > 0.95) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        //a is just any vector that's not parallel to normal
        vec3 t1 = unit_vector(cross(a, n));
        vec3 t2 = cross(n, t1);

        //we want to multiply v_local by the inverse of [t1 | t2 | n]
        //bc its an orthonormal basis the inverse is just the transpose so now t1, t2, n are the rows
        return vec3(dot(v_local, t1), dot(v_local, t2), dot(v_local, n));
    }
    vec3 render_to_local(const vec3& v_render) const
    {
        //convert to world space by creating an orthonormal basis at the hit_record point, with up = normal
        const vec3& n = rec.normal;
        vec3 a = (std::abs(rec.normal.x()) > 0.95) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        //a is just any vector that's not parallel to normal
        vec3 t1 = unit_vector(cross(a, n));
        vec3 t2 = cross(n, t1);

        return v_render.x() * t1 + v_render.y() * t2 + v_render.z() * n;
    }
};

#endif //BXDF_H
