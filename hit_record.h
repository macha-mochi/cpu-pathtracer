//
// Created by Faye Yu on 12/27/25.
//

#ifndef HIT_RECORD_H
#define HIT_RECORD_H

#include "vec3.h"
class light;
class material;

class hit_record{
public:
    point3 p;
    vec3 normal; //assumed to be unit
    shared_ptr<material> mat;
    double t;
    bool front_face;
    double incident_eta = 1.0; //ior of medium ray was traveling through BEFORE hit, 1 by default
    bool hit_light = false;
    light* light_source;

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
    vec3 outward_normal() const
    {
        return front_face ? normal : -normal;
    }
};

#endif //HIT_RECORD_H
