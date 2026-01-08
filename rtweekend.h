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
#include <omp.h>
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

    thread_local std::random_device rd;
    thread_local std::mt19937 generator (rd() + omp_get_thread_num());
    thread_local std::uniform_real_distribution<> distr_double(0.0, 1.0);
    return distr_double(generator);
}
//returns a random real in [min, max)
inline double random_double(double min, double max){
    return min + (max-min)*random_double();
}
//returns a random int in [min, max]
inline int random_int(int min, int max)
{
    thread_local std::random_device rd;
    thread_local std::mt19937 generator (rd() + omp_get_thread_num());
    std::uniform_int_distribution<> distr_int(min, max);
    return distr_int(generator);
}
//common headers
#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "interval.h"

#endif //RTWEEKEND_H
