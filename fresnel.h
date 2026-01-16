//
// Created by Faye Yu on 12/19/25.
//

#ifndef FRESNEL_H
#define FRESNEL_H
#include "spectrum.h"

class fresnel
{
public:
    virtual ~fresnel() = default;

    virtual color evaluate(double cos_i) const = 0;
};

class fresnel_conductor : public fresnel
{
public:
    fresnel_conductor(const double eta_i, const complex_ior& i) : eta_i(eta_i), i(i) {}

    color evaluate(double cos_theta_i) const override
    {
        double r_red = Fr_conductor(cos_theta_i, i.etas[0], i.ks[0]);
        double r_green = Fr_conductor(cos_theta_i, i.etas[1], i.ks[1]);
        double r_blue = Fr_conductor(cos_theta_i, i.etas[2], i.ks[2]);
        return {r_red, r_green, r_blue};
    }
private:
    const complex_ior& i;
    double eta_i;

    double Fr_conductor(double cos_theta_i, double eta_t, double k_t) const{
        //does cos_theta_i always need to be positive like for dielectrics
        double eta = eta_t / eta_i;
        double k = k_t / eta_i;

        double cos2 = cos_theta_i * cos_theta_i;
        double sin2 = 1 - cos2;
        double sin4 = sin2 * sin2;
        double a2minusb2 = eta*eta - k*k - sin2;
        double a2plusb2 = sqrt(a2minusb2 * a2minusb2 + 4 * eta * eta * k * k);

        double a = sqrt((a2plusb2 + a2minusb2)/2);
        double two_a_cos = 2 * a * cos_theta_i;

        double r_perp = (a2plusb2 - two_a_cos + cos2)/
            (a2plusb2 + two_a_cos + cos2);
        double r_para = r_perp * (cos2 * a2plusb2 - two_a_cos * sin2 + sin4)/
            (cos2 * a2plusb2 + two_a_cos * sin2 + sin4);

        return (r_perp + r_para)/2;
    }
};

class fresnel_dielectric : public fresnel
{
public:
    fresnel_dielectric(double eta_above, double eta_below) : eta_a(eta_above), eta_b(eta_below) {}

    color evaluate(double cos_theta_i) const override
    {
        return color(1, 1, 1) * Fr_dielectric(cos_theta_i);
    }
private:
    double eta_a; //a for above
    double eta_b; //b for below

    //eta_i : index of refraction for incident material
    //eta_t : index of refraction for transmitted material
    //returns how much light is reflected (0 = none, 1 = all)
    double Fr_dielectric(double cos_theta_i) const
    {
        double eta_i = eta_a;
        double eta_t = eta_b;
        if (cos_theta_i < 0)
        {
            //the ray is on the inside, swap the etas
            double temp = eta_i;
            eta_i = eta_t;
            eta_t = temp;
            cos_theta_i = std::abs(cos_theta_i); //ensure cos_theta_i is nonneg
        }
        //std::clog << "fresnel is using eta_i: " << eta_i << " eta_t: " << eta_t << std::endl;

        //find cos_theta_t using snell's law
        double sin_theta_i = std::sqrt(std::max(static_cast<double>(0), 1 - cos_theta_i * cos_theta_i));
        double sin_theta_t = eta_i * sin_theta_i / eta_t;
        if (sin_theta_t >= 1)
        {
            return 1; //no solution for snell's law, all light reflected
        }
        double cos_theta_t = std::sqrt(std::max(static_cast<double>(0), 1 - sin_theta_t * sin_theta_t));

        double r_parallel = (eta_t * cos_theta_i - eta_i * cos_theta_t)
        / (eta_t * cos_theta_i + eta_i * cos_theta_t);
        double r_perp = (eta_i * cos_theta_i - eta_t * cos_theta_t)
        / (eta_i * cos_theta_i + eta_t * cos_theta_t);

        return (r_parallel * r_parallel + r_perp * r_perp) / 2;
    }
};

#endif //FRESNEL_H
