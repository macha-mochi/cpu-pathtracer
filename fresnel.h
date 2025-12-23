//
// Created by Faye Yu on 12/19/25.
//

#ifndef FRESNEL_H
#define FRESNEL_H
#include "color.h"

class fresnel
{
public:
    virtual ~fresnel() = default;

    virtual double evaluate(double cos_i) = 0;
};

class fresnel_conductor : public fresnel
{
public:
    fresnel_conductor(double eta_i, double eta_t, double k) : eta_i(eta_i), eta_t(eta_t), k_t(k) {}

    double evaluate(double cos_theta_i) override
    {
        return Fr_conductor(cos_theta_i);
    }
private:
    double eta_i, eta_t, k_t;

    double Fr_conductor(double cos_theta_i){
        /*double eta = eta_t / eta_i;
        double k = k_t / eta_i;

        double cos_squared = cos_theta_i * cos_theta_i;
        double ab_temp = eta*eta - k*k - (1-cos_squared);
        double a2_b2 = sqrt(ab_temp * ab_temp + 4 * eta * eta * k * k);

        double r_perp = a2_b2 - 2 *
        */
        return 1.0;
    }
};

class fresnel_dielectric : public fresnel
{
public:
    fresnel_dielectric(double eta_i, double eta_t) : eta_i(eta_i), eta_t(eta_t) {}

    double evaluate(double cos_theta_i) override
    {
        return Fr_dielectric(cos_theta_i);
    }
private:
    double eta_i;
    double eta_t;

    //eta_i : index of refraction for incident material
    //eta_t : index of refraction for transmitted material
    //returns how much light is reflected (0 = none, 1 = all)
    double Fr_dielectric(double cos_theta_i)
    {
        if (cos_theta_i < 0)
        {
            //the ray is on the inside, swap the etas
            double temp = eta_i;
            eta_i = eta_t;
            eta_t = temp;
            cos_theta_i = std::abs(cos_theta_i); //ensure cos_theta_i is nonneg
        }

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
