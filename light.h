//
// Created by Faye Yu on 12/26/25.
//

#ifndef LIGHT_H
#define LIGHT_H
#include "color.h"
#include "material.h"
#include "quad.h"

class light_sample{
public:
    vec3 wi; //should be from shading point to light
    color emitted;
    double p_solid_angle = 0; //does NOT include the 1/(num_lights)
    light_sample() = default;
    light_sample(const vec3& wi, const color& e, const double p) :
    wi(wi), emitted(e), p_solid_angle(p){};
};

class light : public hittable
{
public:
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override = 0;

    aabb bounding_box() const override = 0;

    std::string to_string() const override{ return "Default light base class";}

    //samples a random point on the light
    virtual light_sample sample(const vec3& x) const
    {
        //given the shading point, returns a light_sample w the info
        return light_sample();
    }
    virtual double pdf(const vec3& x, const vec3& y) const = 0;
};

class quad_light : public light
{
public:
    quad_light(const shared_ptr<quad>& q, const shared_ptr<material>& mat) : q(q), mat(mat) {};

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        bool ans = q->hit(r, ray_t, rec);
        if (ans)
        {
            rec.hit_light = true;
            rec.light_source = const_cast<quad_light*>(this);
        }
        return ans;
    }

    aabb bounding_box() const override
    {
        return q->bounding_box();
    }

    light_sample sample(const vec3& x) const override
    {
        vec3 y = q->get_random_point();
        vec3 wi = unit_vector(y-x);
        //use -wi bc wi is from surface to light
        double cos_theta_y = dot(-wi, q->n()); //shouldn't need to divide bc theyre both unit vectors
        if (cos_theta_y <= 0.0)
        {
            //backface or parallel, no light should be reaching the point
            return light_sample(wi, color(0, 0, 0), 0);
        }
        return light_sample(wi, mat->emitted(), pdf(x, y));
    }
    //x: shading point, y: point on light, but pdf is returned in solid angle, before num_lights accounted for
    double pdf(const vec3& x, const vec3& y) const override
    {
        double p_a = 1.0/q->get_area();
        vec3 wi = unit_vector(y-x);
        double cos_theta_y = dot(-wi, q->n()); //shouldn't need to divide bc theyre both unit vectors
        if (cos_theta_y <= 0.0) //on the backside of the light or on the exact same plane, which should not be lit
        {
            return 0;
        }
        return p_a * (x-y).length_squared() / cos_theta_y;
    }
    std::string to_string() const override{
        return "Quad light | Quad: \n" + q->to_string() + "\nEmit: " + mat->emitted().to_string();
    }
private:
    shared_ptr<quad> q;
    shared_ptr<material> mat;
};

#endif //LIGHT_H
