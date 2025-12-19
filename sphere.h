//
// Created by Faye Yu on 10/26/25.
//

#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"

class sphere : public hittable{
public:
    sphere(const point3& center, double radius, const shared_ptr<material>& mat)
    : center(center), radius(std::fmax(0, radius)), mat(mat)
    {
        //Stationary sphere
        //TODO change if you ever add movement
        auto rvec = vec3(radius, radius, radius);
        bbox = aabb(center - rvec, center + rvec);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        vec3 oc = center - r.origin();

        //these 3 vars derived from equation of a sphere and a ray hitting that sphere
        auto a = r.direction().length_squared(); //also equals r dot itself
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = h*h - a*c;
        if (discriminant < 0) return false;

        auto sqrtd = std::sqrt(discriminant);

        //find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        //if the ray we sent out is Q + td where Q is start pos and d is direction,
        //we want to find t. we first try subtracting
        if (!ray_t.surrounds(root))
        {
            root = (h + sqrtd)/a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;
        rec.incident_eta = r.current_ior();

        return true;
    }
    aabb bounding_box() const override {return bbox;};
private:
    point3 center;
    double radius;
    shared_ptr<material> mat;
    aabb bbox;
};



#endif //SPHERE_H
