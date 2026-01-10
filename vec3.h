//
// Created by Faye Yu on 10/12/25.
//

#ifndef VEC3_H
#define VEC3_H

#include <random>

class vec3 {
public:
    double e[3];

    vec3() : e{0, 0, 0} {}
    vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

    double x() const { return e[0];}
    double y() const { return e[1];}
    double z() const { return e[2];}

    vec3 operator-() const {return vec3(-e[0], -e[1], -e[2]);}
    double operator[](int i) const{ return e[i];}
    double& operator[](int i) {return e[i];}

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0]; e[1] += v.e[1]; e[2] += v.e[2];
        return *this;
    }
    vec3& operator*=(double t) {
        e[0]*= t;
        e[1]*= t;
        e[2]*= t;
        return *this;
    }
    vec3& operator/=(double t) {
        return *this *= 1/t;
    }
    double length() const {
        return std::sqrt(length_squared());
    }
    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }
    bool near_zero() const
    {
        //Returns true if the vec is close to 0 in all dir
        auto s = 1e-8;
        return(std::fabs(e[0]) < s) && (std::fabs(e[1]) < s) && (std::fabs(e[2]) < s);
    }
    static vec3 random()
    {
        return vec3(random_double(), random_double(), random_double());
    }
    static vec3 random(double min, double max)
    {
        //basically means you have a vector whose point can be anywhere
        //within a sphere with a hole cut out of it
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }
    std::string to_string() const
    {
        return "{" + std::to_string(e[0]) + ", " + std::to_string(e[1]) + ", " + std::to_string(e[2]) + "}";
    }
};

//point3 is just an alias for vec3, but useful for geometric clarity in the code
using point3 = vec3;

//Vector utility functions
inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}
inline vec3 operator+(const vec3& u, const vec3& v) {
    return(vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]));
}
inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}
inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3(u.e[0]*v.e[0], u.e[1]*v.e[1], u.e[2]*v.e[2]);
}
inline vec3 operator*(double t, const vec3& v) {
    return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}
inline vec3 operator*(const vec3& v, double t) {
    return t * v;
}
inline vec3 operator/(const vec3& v, double t) {
    return (1/t) * v;
}
inline double dot(const vec3& u, const vec3& v) {
    return u.e[0]*v.e[0] + u.e[1]*v.e[1] + u.e[2]*v.e[2];
}
inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
            u.e[2] * v.e[0] - u.e[0] * v.e[2],
            u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}
inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}
inline vec3 clamp(const vec3& v, interval i)
{
    return vec3(i.clamp(v.e[0]), i.clamp(v.e[1]), i.clamp(v.e[2]));
}
inline vec3 random_in_unit_disk()
{
    while (true)
    {
        auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
        if (p.length_squared() < 1) return p;
    }
}
inline vec3 cos_weighted_random_in_hemisphere()
{
    double r1 = random_double();
    double r2 = random_double();

    double r = sqrt(r1);
    double phi = 2 * M_PI * r2;
    return vec3(r * cos(phi), r * sin(phi), sqrt(1 - r1));
}
inline vec3 random_unit_vector()
{
    //Generate a random vector whose tip is on the unit sphere
    while(true){
        vec3 v = vec3::random(-1, 1);
        double lensq = v.length_squared();
        if (1e-160 < lensq && lensq <= 1) return v / sqrt(lensq);
    }
}
inline vec3 random_on_hemisphere(const vec3& normal){
    vec3 rand = random_unit_vector();
    if(dot(rand, normal) > 0.0) return rand;
    return -rand;
}
//this func assumes that v is pointed at the shading point and not away from it
inline vec3 reflect(const vec3& v, const vec3& n)
{
    return v - 2*dot(n, v)*n;
}
//returns false if no valid transmission, otherwise the transmission direction is wt
//if wi and n are not in the same hemi, n will be flipped so they are
//wt is a unit vector
inline bool refract(const vec3& wi, const vec3& norm, double etai_over_etat, vec3& wt)
{
    //flip the normal if wi and n are not in the same hemisphere
    vec3 n = dot(wi, norm) < 0 ? -norm : norm;
    double cos_theta_i = std::fmin(dot(n, wi), 1.0);
    double sin2_theta_i = 1.0 - cos_theta_i * cos_theta_i;
    double sin2_theta_t = etai_over_etat * etai_over_etat * sin2_theta_i;

    if (sin2_theta_t > 1) return false; //no solution for snell's law, sin_theta_t > 1

    double cos_theta_t = sqrt(1 - sin2_theta_t);
    vec3 wt1 = etai_over_etat * -wi + (etai_over_etat * cos_theta_i - cos_theta_t) * n;
    wt.e[0] = wt1.e[0];
    wt.e[1] = wt1.e[1];
    wt.e[2] = wt1.e[2];
    return true;
}

#endif //VEC3_H
