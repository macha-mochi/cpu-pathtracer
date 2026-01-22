//
// Created by Faye Yu on 12/19/25.
//

#ifndef BXDF_H
#define BXDF_H
#include <utility>

#include "fresnel.h"
#include "hit_record.h"
#include "trowbridge_reitz_distribution.h"

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
    SpecularReflTran = Specular | Reflection | Transmission,
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
    virtual bsdf_sample sample(const vec3& wo)
    {
        return bsdf_sample();
    }
    std::string flags_to_string()
    {
        std::string s;
        if (is_reflective(flags))
        {
            s+="Reflective";
        }else if (is_transmission(flags))
        {
            s+="Transmission";
        }
        s+= " ";
        if (is_diffuse(flags))
        {
            s+="Diffuse";
        }else if (is_glossy(flags))
        {
            s+="Glossy";
        }else if (is_specular(flags))
        {
            s+="Specular";
        }
        return s;
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
    bsdf_sample sample(const vec3& wo) override
    {
        //generate a wi using cosine weighted hemisphere sampling
        vec3 wi = cos_weighted_random_in_hemisphere();

        //populate a bxdf_sample with calculated wi, f, pdf, is_delta
        return bsdf_sample(wi, f_s(wo, wi), pdf(wo, wi), false);
    }
private:
    color albedo;
};

class specular_reflection : public bxdf
{
public:
    specular_reflection(const color& r, const fresnel *f) : r_scale_factor(r), fresnel(f)
    {
        flags = SpecularReflection;
    }
    color f_s(const vec3& wo, const vec3& wi) const override
    {
        return color(0,0,0); //nothing scatters in any direction except the one light reflects in
    }
    double pdf(const vec3& wo, const vec3& wi) const override
    {
        return 0.0; //we don't use a pdf for speculars bc there's only one correct direction we can calculate
    }
    //wo is a unit vector in render space alr, in same hemisphere as normal pointing away from shading point
    bsdf_sample sample(const vec3& wo) override
    {
        //theta is angle between incoming ray and normal
        double cos_theta = std::fmin(dot(wo, n), 1.0);
        cos_theta = std::abs(cos_theta);

        color reflectance = fresnel->evaluate(cos_theta);
        vec3 wi = vec3(-wo.x(), -wo.y(), wo.z());
        color f_s = reflectance / cos_theta; //im p sure cos_theta_i = cos_theta_r
        return bsdf_sample(wi, r_scale_factor * f_s, 1, true);
    }
private:
    color r_scale_factor;
    const fresnel *fresnel;
};

class specular_transmission : public bxdf
{
public:
    specular_transmission(color& t, double eta_a, double eta_b) : t_scale_factor(t), eta_a(eta_a), eta_b(eta_b),
    fresnel(eta_a, eta_b)
    {
        flags = SpecularTransmission;
    }
    color f_s(const vec3& wo, const vec3& wi) const override
    {
        return color(0,0,0); //nothing scatters in any direction except the one light reflects in
    }
    double pdf(const vec3& wo, const vec3& wi) const override
    {
        return 0.0; //we don't use a pdf for speculars bc there's only one correct direction we can calculate
    }
    //wo is a unit vector in render space alr, is pointing away from shading point same hemi as normal
    bsdf_sample sample(const vec3& wo) override
    {
        //theta is angle between incoming ray and normal
        double cos_theta = std::fmin(dot(wo, n), 1.0);
        bool entering = cos_theta > 0; //cos_theta is just z component of -wo
        double eta_i = entering ? eta_a : eta_b;
        double eta_t = entering ? eta_b : eta_a;
        cos_theta = std::abs(cos_theta);

        double reflectance = fresnel.evaluate(cos_theta).x(); //dielectric so all channels should be same
        vec3 wi;
        if (refract(wo, n, eta_i/eta_t, wi))
        {
            double cos_theta_i = dot(wi, n); //wi and n should both be unit vectors
            double f_s = (1-reflectance) / std::abs(cos_theta_i);
            f_s *= (eta_i * eta_i)/(eta_t* eta_t); //account for "compressing" of light rays as it transmits
            return bsdf_sample(wi, t_scale_factor * f_s, 1, true);
        }else
        {
            return bsdf_sample();
        }
    }
private:
    color t_scale_factor;
    double eta_a, eta_b; //above = the side the surface normal is in, below = the other side
    fresnel_dielectric fresnel;
};

class fresnel_specular : public bxdf
{
public:
    fresnel_specular(const color& r, const color& t, double eta_a, double eta_b) :
    r_scale_factor(r), t_scale_factor(t), eta_a(eta_a), eta_b(eta_b), fresnel(eta_a, eta_b)
    {
        flags = SpecularReflTran;
    }
    color f_s(const vec3& wo, const vec3& wi) const override
    {
        return color(0,0,0); //nothing scatters in any direction except the one light reflects in
    }
    double pdf(const vec3& wo, const vec3& wi) const override
    {
        return 0.0; //we don't use a pdf for speculars bc there's only one correct direction we can calculate
    }
    //wo is a unit vector in render space alr, pointing away from shading point
    bsdf_sample sample(const vec3& wo) override
    {
        double cos_theta = std::fmin(dot(wo, n), 1.0);
        bool entering = cos_theta > 0; //cos_theta is just z component of wo
        //cos_theta is cosine with the shading normal, rec.normal, which always faces same as ray, so entering should
        //always be true. so technically i dont need this code but if i implement normal maps (in hit_record i expect)
        //and this causes rec.normal and wo to no longer be always facing the same this will be helpful for ensuring
        //no back face problems
        double eta_i = entering ? eta_a : eta_b;
        double eta_t = entering ? eta_b : eta_a;

        //std::clog << "in bsdf | eta_i: " << eta_i << " eta_t: " << eta_t << std::endl;
        double reflectance = fresnel.evaluate(cos_theta).x(); //is dielectric so you know all channels r the same (probably)
        //std::clog << fresnel.evaluate(cos_theta) << std::endl;
        vec3 wi;
        color f_s;
        double pdf;
        if (random_double() < reflectance) //reflect
        {
            wi = vec3(-wo.x(), -wo.y(), wo.z());
            f_s = r_scale_factor * (reflectance / std::abs(cos_theta)); //im p sure cos_theta_i = cos_theta_r
            //std::clog << "reflected" << std::endl;
            pdf = reflectance;
        }else //refract
        {
            refract(wo, n, eta_i/eta_t, wi);
            double cos_theta_i = dot(wi, n); //wi and n should both be unit vectors
            //std::clog << "refract -> eta ratio squared: " << (eta_i * eta_i)/(eta_t* eta_t) << " cos theta i: " << cos_theta_i << std::endl;
            f_s = t_scale_factor * (eta_i * eta_i)/(eta_t* eta_t) * (1-reflectance) / std::abs(cos_theta_i);
            //account for "compressing" of light rays as it transmits
            pdf = 1 - reflectance;
        }
        //std::clog << "returning sample with f_s: " << f_s << " and pdf: " << pdf << std::endl;
        return bsdf_sample(wi, f_s, pdf, true);
    }
private:
    color r_scale_factor;
    color t_scale_factor;
    double eta_a, eta_b; //above = the side the surface normal is in, below = the other side
    fresnel_dielectric fresnel;
};

class conductor : public bxdf
{
public:
    conductor(const trowbridge_reitz_distribution& d, const color& albedo, fresnel_conductor f) :
    mf_dist(d), albedo(albedo), fresnel(std::move(f))
    {
        flags = mf_dist.act_as_smooth() ? SpecularReflection : GlossyReflection;
    }
    color f_s(const vec3& wo, const vec3& wi) const override
    {
        if (mf_dist.act_as_smooth())
        {
            return color(0,0,0); //nothing scatters in any direction except the one light reflects in
        }
        vec3 wm = (wo+wi);
        if (wm.length_squared() <= 1e-8) return color(0, 0, 0); //wo and wi in opposite directions
        wm = unit_vector(wm);
        if (dot(wm, n) < 0) wm = -wm;

        double cos_theta_o = abs(wo.z());
        double cos_theta_i = abs(wi.z());
        if (cos_theta_o <= 1e-9 || cos_theta_i <= 1e-9) return color(0, 0, 0); //avoid nans

        color f_temp = mf_dist.D(wm) * fresnel.evaluate(dot(wo, wm)) * mf_dist.G(wo, wi);
        return f_temp * 1 / (4 * cos_theta_o * cos_theta_i);
    }
    double pdf(const vec3& wo, const vec3& wi) const override
    {
        if (mf_dist.act_as_smooth())
        {
            return 0.0; //we don't use a pdf for speculars bc there's only one correct direction we can calculate
        }

        vec3 wm = (wo+wi);
        if (wm.length_squared() <= 1e-8) return 0; //wo and wi in opposite directions
        wm = unit_vector(wm);
        if (dot(wm, n) < 0) wm = -wm;
        return mf_dist.pdf(wo, wm) / 4 * dot(wo, wm);
    }
    //wo is a unit vector in render space alr, pointing away from shading point
    bsdf_sample sample(const vec3& wo) override
    {
        if (mf_dist.act_as_smooth())
        {
            //theta is angle between incoming ray and normal
            double cos_theta = std::fmin(dot(wo, n), 1.0);
            cos_theta = std::abs(cos_theta);

            color reflectance = fresnel.evaluate(cos_theta);
            vec3 wi = vec3(-wo.x(), -wo.y(), wo.z());
            color f_s = reflectance / cos_theta; //im p sure cos_theta_i = cos_theta_r
            return bsdf_sample(wi, albedo * f_s, 1, true);
        }

        vec3 wm = mf_dist.sample_wm(wo);
        vec3 wi = reflect(wo, wm);
        return bsdf_sample(wi, albedo * f_s(wo, wi), pdf(wo, wi), false);
    }
private:
    const trowbridge_reitz_distribution& mf_dist;
    color albedo;
    fresnel_conductor fresnel;
};

class bsdf
{
public:
    std::vector<std::unique_ptr<bxdf>> bxdfs;
    const hit_record& rec;

    bsdf(const hit_record& rec) : rec(rec)
    {
        //create an orthonormal basis at the hit_record point, with up = normal
        n = rec.normal; //rec.outward_normal();
        vec3 a = (std::abs(n.x()) > 0.95) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        //a is just any vector that's not parallel to normal
        t1 = unit_vector(cross(a, n));
        t2 = unit_vector(cross(n, t1));
    };

    template <typename T, typename... Args>
    void add(Args&&... args)
    {
        bxdfs.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
    //the physically correct f_s from each bxdf, takes wo and wi in WORLD SPACE, to be called for mis
    color f_s(const vec3& wo, const vec3& wi) const
    {
        //NOTE TO SELF: might have a problem with reflection/transmission if u need to filter out certain non-delta lobes
        //or if its in the wrong hemisphere or smth
        return f_s_render(local_to_render(wo), local_to_render(wi));
    }
    //the marginal pdf for wi, equal to sum(i = 1 -> k) Pr(choosing kth lobe) * Pr(getting wi from the kth lobe)
    //takes wo and wi in WORLD SPACE, to be called for mis
    double pdf(const vec3& wo, const vec3& wi) const
    {
        return pdf_render(local_to_render(wo), local_to_render(wi));
    }
    //assumes wo is a unit vector in world space
    bsdf_sample sample(const vec3& wo_world) const
    {
        //TODO i will use uniform for now and then switch to balance heuristic or whatever later so i can compare
        int rand_ind = random_int(0,static_cast<int>(bxdfs.size()) - 1);
        bxdf& b = *bxdfs[rand_ind];
        vec3 wo = local_to_render(wo_world);
        bsdf_sample sample_for_dir = b.sample(wo);
        vec3 wi = sample_for_dir.wi;
        vec3 wi_world = render_to_local(wi);
        if (sample_for_dir.is_delta)
        {
            return bsdf_sample(wi_world, sample_for_dir.f, sample_for_dir.pdf, true);
            //if it's a delta distribution don't add up any other bxdfs into f and pdf
        }
        return bsdf_sample(wi_world, f_s_render(wo, wi), pdf_render(wo, wi), false);
    }
    vec3 local_to_render(const vec3& v_local) const
    {
        //we want to multiply v_local by the inverse of [t1 | t2 | n]
        //bc its an orthonormal basis the inverse is just the transpose so now t1, t2, n are the rows
        return vec3(dot(v_local, t1), dot(v_local, t2), dot(v_local, n));
    }
    vec3 render_to_local(const vec3& v_render) const
    {
        return v_render.x() * t1 + v_render.y() * t2 + v_render.z() * n;
    }
    std::string flags_to_string()
    {
        std::string s;
        for (auto & bxdf : bxdfs)
        {
            s+=bxdf->flags_to_string();
            s+="\n";
        }
        return s;
    }
private:
    //an orthonormal basis in world space at the hit_record point, with up = hit_rec.outward normal
    vec3 n;
    vec3 t1;
    vec3 t2;
    //the physically correct f_s from each bxdf, takes wo and wi in RENDER SPACE
    color f_s_render(const vec3& wo, const vec3& wi) const
    {
        //NOTE TO SELF: might have a problem with reflection/transmission if u need to filter out certain non-delta lobes
        //or if its in the wrong hemisphere or smth
        color result = color(0, 0, 0);
        for (const auto & bxdf : bxdfs)
        {
            result+=bxdf->f_s(wo, wi); //if is delta, this will be 0 so issok
        }
        return result;
    }
    //the marginal pdf for wi, equal to sum(i = 1 -> k) Pr(choosing kth lobe) * Pr(getting wi from the kth lobe)
    //takes wo and wi in RENDER SPACE
    double pdf_render(const vec3& wo, const vec3& wi) const
    {
        double w_k = 1.0/bxdfs.size(); //TODO change to not be uniform later
        double result = 0.0;
        for (const auto & bxdf : bxdfs)
        {
            result+=w_k * bxdf->pdf(wo, wi); //if is delta, this will be 0 so issok
        }
        return result;
    }
};


#endif //BXDF_H
