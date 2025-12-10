//
// Created by Faye Yu on 11/9/25.
//

#ifndef AABB_H
#define AABB_H
#include "interval.h"

class aabb
{
public:
    interval x, y, z;
    aabb() = default; //default aabb is empty bc intervals empty by default
    aabb(const interval& x, const interval& y, const interval& z) : x(x), y(y), z(z)
    {
        pad_to_minimum();
    };

    aabb(const point3& a, const point3& b)
    {
        //treat a and b as extrema for bounding box
        x = (a[0] <= b[0]) ? interval(a[0],b[0]) : interval(b[0],a[0]);
        y = (a[1] <= b[1]) ? interval(a[1],b[1]) : interval(b[1],a[1]);
        z = (a[2] <= b[2]) ? interval(a[2],b[2]) : interval(b[2],a[2]);

        pad_to_minimum();
    }
    aabb(const aabb& b1, const aabb& b2){
        x = interval(b1.x, b2.x);
        y = interval(b1.y, b2.y);
        z = interval(b1.z, b2.z);
    }
    const interval& axis_interval(int n) const
    {
        if (n == 0) return x;
        if (n == 1) return y;
        return z;
    }
    bool hit(const ray& r, interval ray_t) const //2nd paran is the time interval the ray spends in the aabb
    {
        const point3& origin = r.origin();
        const point3& direction = r.direction();

        for (int axis = 0; axis < 3; axis++){
            const interval& ax = axis_interval(axis);

            auto t_enter = (ax.min - origin[axis])/direction[axis];
            auto t_exit = (ax.max - origin[axis])/direction[axis];

            if (t_enter > t_exit)
            {
                auto temp = t_enter;
                t_enter = t_exit;
                t_exit = temp;
            }
            if (t_enter > ray_t.min) ray_t.min = t_enter;
            if (t_exit < ray_t.max) ray_t.max = t_exit;

            if (ray_t.max <= ray_t.min) return false;
        }
        return true;
    }
    int longest_axis() const
    {
        //returns index of longest axis of the bounding box
        if (x.size() > y.size()) return x.size() > z.size() ? 0 : 2;
        else return y.size() > z.size() ? 1 : 2;
    }
    double surface_area() const
    {
        return 2*(x.size()*y.size() + x.size()*z.size() + y.size()*z.size());
    }
    static const aabb empty, universe;
    point3 get_centroid() const
    {
        return {(x.min + x.max)/2, (y.min + y.max)/2, (z.min + z.max)/2};
    }
private:
    void pad_to_minimum()
    {
        //adjust aabb so no side is narrower than some delta
        double delta = 0.0001;
        if (x.size() < delta) x = x.expand(delta);
        if (y.size() < delta) y = y.expand(delta);
        if (z.size() < delta) z = z.expand(delta);
    }
};

const aabb aabb::empty = aabb(interval::empty, interval::empty, interval::empty);
const aabb aabb::universe = aabb(interval::universe, interval::universe, interval::universe);

aabb operator+(const aabb& bbox, const vec3& offset)
{
    return aabb(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z());
}
aabb operator+(const vec3& offset, const aabb& bbox)
{
    return bbox + offset;
}

#endif //AABB_H
