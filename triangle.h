//
// Created by Faye Yu on 11/21/25.
//

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "triangle_mesh.h"

class triangle : public hittable
{
public:
    /**
     * Make a triangle, assuming the v0, v1, v2 are in CCW order
     * @param v0
     * @param v1
     * @param v2
     * @param mat
     */
    triangle(const point3& v0, const point3& v1, const point3& v2, const shared_ptr<material>& mat)
    : v0(v0), v1(v1), v2(v2), mat(mat)
    {
        vec3 n = cross(v1 - v0, v2 - v0);
        normal = unit_vector(n);
        D = dot(normal, v0);

        aabb b1 = aabb(v0, v1);
        aabb b2 = aabb(v1, v2);
        bbox = aabb(b1, b2);
    }
    aabb bounding_box() const override { return bbox;}
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        auto denom = dot(normal, r.direction());
        if (std::fabs(denom) < 1e-8) return false; //ray is parallel to the plane the triangle is in

        auto t = (D - dot(normal, r.origin()))/denom;
        if (!ray_t.contains(t)) return false; //intersection time is outside valid interval (ex: neg)

        auto hit_point = r.at(t);

        vec3  v0v1 = v1 - v0;
        vec3 v0P = hit_point - v0;
        if (dot(cross(v0v1, v0P), normal) < 0){ return false;}
        vec3 v1v2 = v2 - v1;
        vec3 v1P = hit_point - v1;
        if (dot(cross(v1v2, v1P), normal) < 0){ return false;}
        vec3 v2v0 = v0 - v2;
        vec3 v2P = hit_point - v2;
        if (dot(cross(v2v0, v2P), normal) < 0){ return false;}

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.mat = mat;

        return true;
    }
    std::string to_string() const
    {
        return v0.to_string() + " " + v1.to_string() + " " + v2.to_string();
    }

private:
    const point3 v0;
    const point3 v1;
    const point3 v2;
    vec3 normal;
    shared_ptr<material> mat;
    aabb bbox;
    double D;
};

#endif //TRIANGLE_H
