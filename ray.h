//
// Created by Faye Yu on 10/12/25.
//

#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction)
    {
        eta = 1.0; //for air
    }

    const point3& origin() const {return orig;}
    const vec3& direction() const {return dir;}
    double current_ior() const {return eta;}
    void set_eta(const double e)
    {
        eta = e;
    }

    point3 at(double t) const {
        return orig + t*dir;
    }

private:
    point3 orig;
    vec3 dir;
    double eta; //ior of current medium you're in
};

#endif //RAY_H
