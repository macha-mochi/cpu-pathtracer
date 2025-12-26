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

class light : hittable
{
public:
    virtual ~light() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;

    //samples a random point on the light
    virtual light_sample sample(const vec3& x) const
    {
        //given the shading point, returns a light_sample w the info
        return light_sample();
    }
};

class quad_light : public light
{
public:
    quad_light(const shared_ptr<quad>& q, const shared_ptr<material>& mat) : q(q), mat(mat) {};

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        return q->hit(r, ray_t, rec);
    }

    aabb bounding_box() const override
    {
        return q->bounding_box();
    }

    light_sample sample(const vec3& x) const override
    {
        vec3 y = q->get_random_point();
        double p_a = 1.0/q->get_area();
        vec3 wi = unit_vector(y-x);
        //use -wi bc wi is from surface to light
        double cos_theta_y = dot(-wi, q->n()); //shouldn't need to divide bc theyre both unit vectors
        if (cos_theta_y < 0)
        {
            //backface, no light should be reaching the point
            return light_sample(wi, color(0, 0, 0), 0);
        }
        return light_sample(wi, mat->emitted(), p_a * (x-y).length_squared() / cos_theta_y);
    }
private:
    shared_ptr<quad> q;
    shared_ptr<material> mat;
};

#endif //LIGHT_H
