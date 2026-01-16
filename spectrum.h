//
// Created by Faye Yu on 1/10/26.
//

#ifndef SPECTRUM_H
#define SPECTRUM_H
#include <vector>

struct complex_ior
{
    //using 650, 550, and 450 nm for r, g, b
    std::vector<double> etas;
    std::vector<double> ks;
};

namespace Metal
{
    inline const complex_ior steel = {
            {2.441, 2.0440, 1.6778},
            {4.1820, 3.6732, 3.1170}
    };
    inline const complex_ior silver = {
            {0.15943, 0.14512, 0.13547},
            {3.9291, 3.1900, 2.3808}
    };
    inline const complex_ior gold = {
            {0.18299, 0.42108, 1.3734},
            {3.4242, 2.3459, 1.7704}
    };
    inline const complex_ior copper = {
            {0.27105, 0.67693, 1.3164},
            {3.6092, 2.6248, 2.2921}
    };
}

#endif //SPECTRUM_H
