//
// Created by Faye Yu on 11/13/25.
//

#ifndef QUAD_H
#define QUAD_H

#include "hittable.h"

class quad : public hittable
{
public:
    quad(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material>mat) :
    Q(Q), u(u), v(v), mat(mat)
    {
        vec3 n = cross(u, v);
        normal = unit_vector(n); //find the normal vector
        D = dot(normal, Q);
        w = n / dot(n, n);

        set_bounding_box();
    }
    virtual void set_bounding_box()
    {
        aabb b1 = aabb(Q, Q + u + v);
        aabb b2 = aabb(Q + u, Q + v);
        bbox = aabb(b1, b2);
    }
    aabb bounding_box() const override { return bbox;}
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        auto denom = dot(normal, r.direction());
        if (std::fabs(denom) < 1e-8) return false; //ray is parallel to the plane

        auto t = (D - dot(normal, r.origin()))/denom;
        if (!ray_t.contains(t)) return false; //intersection time is outside valid interval

        auto hit_point = r.at(t);
        auto p = hit_point - Q;
        auto alpha = dot(w, cross(p, v));
        auto beta = dot(w, cross(u, p));
        //check raytracing book for math

        auto unit_interval = interval(0, 1);
        if (!(unit_interval.contains(alpha) && unit_interval.contains(beta)))
        {
            return false;
        }

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.mat = mat;

        return true;
    }
private:
    point3 Q;
    vec3 u;
    vec3 v;
    vec3 w;
    shared_ptr<material> mat;
    aabb bbox;
    vec3 normal;
    double D;
};

#endif //QUAD_H
