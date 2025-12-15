//
// Created by Faye Yu on 10/26/25.
//

#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <random>

//c++ std usings
using std::make_shared;
using std::shared_ptr;

//constants
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

//utility funcs
inline double degrees_to_radians(double degrees)
{
    return degrees * pi / 180.0;
}

inline double random_double()
{
    //returns a random real in [0, 1)
    //return std::rand() / (RAND_MAX + 1.0);

    static std::mt19937 generator (std::random_device{}());
    static std::uniform_real_distribution<> distribution(0.0, 1.0);
    return distribution(generator);
}
inline double random_double(double min, double max){
    //returns a random real in [min, max)
    return min + (max-min)*random_double();
}
//common headers
#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "interval.h"

#endif //RTWEEKEND_H
