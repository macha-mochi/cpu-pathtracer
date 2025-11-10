//
// Created by Faye Yu on 10/26/25.
//

#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"

class material;

class hit_record{
    public:
        point3 p;
        vec3 normal;
        shared_ptr<material> mat;
        double t;
        bool front_face;

        void set_face_normal(const ray& r, const vec3& outward_normal)
        {
            //sets the hit record normal vector
            //outward_normal assumed to be unit
            front_face = dot(r.direction(), outward_normal) < 0;
            normal = front_face ? outward_normal : -outward_normal;
            //if outward normal against ray, ray is outside sphere
            //if outward normal in the same dir as ray, ray is inside sphere
            //outward normal is from center of sphere to point of intersection
        }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;
};



#endif //HITTABLE_H
