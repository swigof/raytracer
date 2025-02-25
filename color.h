#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "interval.h"

using color = vec3;

inline double linear_to_gamma(double linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);

    return 0;
}

uint32_t get_color(const color& pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Replace NaN components with zero.
    if (std::isnan(r))
        r = 0.0;
    if (std::isnan(g))
        g = 0.0;
    if (std::isnan(b))
        b = 0.0;


    // Apply a linear to gamma transform for gamma 2
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    // Translate the [0,1] component values to the byte range [0,255].
    static const interval intensity(0.000, 0.999);
    uint8_t rbyte = uint8_t(256 * intensity.clamp(r));
    uint8_t gbyte = uint8_t(256 * intensity.clamp(g));
    uint8_t bbyte = uint8_t(256 * intensity.clamp(b));
    uint8_t abyte = 255;

    return (abyte << 24) | (bbyte << 16) | (gbyte << 8) | rbyte;
}

#endif
