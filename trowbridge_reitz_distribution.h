//
// Created by Faye Yu on 1/18/26.
//

#ifndef TROWBRIDGEREITZDISTRIBUTION_H
#define TROWBRIDGEREITZDISTRIBUTION_H
#include <algorithm>

#include "vec3.h"

class trowbridge_reitz_distribution
{
public:
    trowbridge_reitz_distribution(float ax, float ay) : alpha_x(ax), alpha_y(ay) {};

    bool act_as_smooth() const
    {
        return std::max(alpha_x, alpha_y) < 1e-3;
    }

    //will also use (0, 0, 1) as normal. returns the (to be integrated) area with normal wm
    double D(const vec3& wm) const
    {
        double cos2theta_m = wm.z() * wm.z();
        double cos4theta_m = cos2theta_m * cos2theta_m;
        double sin2theta_m = 1 - cos2theta_m;
        double tan2theta_m = sin2theta_m / cos2theta_m;

        double cos2phi_m = wm.x() * wm.x() / sin2theta_m;
        double sin2phi_m = 1 - cos2phi_m;

        double temp = (1 + tan2theta_m * (cos2phi_m / (alpha_x * alpha_x) + sin2phi_m / (alpha_y * alpha_y)));

        return 1 / (M_PI * alpha_x * alpha_y * cos4theta_m *
            temp * temp);
    }
    //w is the viewing direction, returns proportion of microfacets that are visible
    //problem: can overestimate amount of shadowing/masking
    double G1(const vec3& w) const
    {
        return 1 / (1 + lambda(w));
    }
    double G(const vec3& wo, const vec3& wi) const
    {
        return 1 / (1 + lambda(wo) + lambda(wi));
    }
    //w: viewing direction, wm: microfacet normal
    //returns projected area of forward facing normals
    double D_visible_normals(const vec3& w, const vec3& wm) const
    {
        return G1(w) / std::abs(w.z()) * D(wm) * std::max(0.0, dot(w, wm));
    }
    //the probability a ray hits a microfacet w a certain normal is
    //equal to the proportion of visible area on the surface w that normal over dA * |costheta|
    double pdf(const vec3& w, const vec3& wm) const {return D_visible_normals(w, wm);}
    vec3 sample_wm(const vec3& w) const
    {
        //convert w from ellipsoid space (assumed) to hemispherical
        vec3 w_hemi = unit_vector(vec3(alpha_x * w.x(), alpha_y * w.y(), w.z()));
        if (w_hemi.z() < 0) //pointing into the hemisphere, we flip so it's pointing out
        {
            w_hemi = -w_hemi;
        }
        double cos_theta = w_hemi.z();
        point3 p = random_in_unit_disk();

        //create orthonormal basis with w_hemi as normal
        vec3 n = vec3(0, 0, 1);
        vec3 t1 = (w_hemi.z() < 0.9999f) ? cross(n, w_hemi) : vec3(1, 0, 0); //t1 is perpendicular to n AND w_hemi
        vec3 t2 = cross(w_hemi, t1);

        //transform p onto the squashed projected disk
        double h = std::sqrt(1 - p.x() * p.x());
        //we don't need to transform p.x, only p.y
        double new_y = p.y() * (1 + cos_theta)/2 + h / 2 * (1 - cos_theta);
        //(new_x, new_y) is a point on the "squashed" projected disk
        double new_z = std::sqrt(std::max(0.0, 1 - p.x() * p.x() - new_y * new_y));
        //(new_x, new_y, new_z) is a point lifted up to unit hemisphere, but guaranteed not to dip below macro surface
        vec3 nh = p.x() * t1 + new_y * t2 + new_z * w_hemi;

        //scale x and y by ellipsoid scaling again, make sure the new direction is not on the base of the sphere
        return unit_vector(vec3(nh.x() * alpha_x, nh.y() * alpha_y, std::max(1e-6, nh.z())));
    }
private:
    double alpha_x, alpha_y;
    vec3 n = vec3(0, 0, 1);

    double lambda(const vec3& w) const
    {
        double alpha2 = 0;
        double cos2theta = w.z() * w.z();
        double sin2theta = 1 - cos2theta;
        double tan2theta = sin2theta / cos2theta;
        if (std::abs(alpha_x - alpha_y) <= 1e-6)
        {
            //basically same
            alpha2 = alpha_x * alpha_y;
        }else
        {
            double cos2phi = w.x() * w.x() / sin2theta;
            double sin2phi = 1 - cos2phi;

            alpha2 = alpha_x * alpha_x * cos2phi + alpha_y * alpha_y * sin2phi;
        }
        return (sqrt(1 + alpha2 * tan2theta) - 1)/2;
    }
};

#endif //TROWBRIDGEREITZDISTRIBUTION_H
