//
// Created by Faye Yu on 10/26/25.
//

#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"

class material;

class hit_record{
    public:
        point3 p;
        vec3 normal;
        shared_ptr<material> mat;
        double t;
        bool front_face;
        double incident_eta = 1.0; //ior of medium ray was traveling through BEFORE hit, 1 by default

        void set_face_normal(const ray& r, const vec3& outward_normal)
        {
            //sets the hit record normal vector
            //outward_normal assumed to be unit
            front_face = dot(r.direction(), outward_normal) < 0;
            normal = front_face ? outward_normal : -outward_normal;
            //if outward normal against ray, ray is outside sphere
            //if outward normal in the same dir as ray, ray is inside sphere
            //outward normal is from center of sphere to point of intersection
        }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;
};
class translate : public hittable
{
    public:
    translate(shared_ptr<hittable> p, const vec3& offset) : object(p), offset(offset)
    {
        bbox = object->bounding_box() + offset;  //dont forget to offset the bounding box
    }
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        //Move ray origin back by offset
        ray offset_ray(r.origin() - offset, r.direction());

        //Determine if intersection occurs with offset
        if (!object->hit(offset_ray, ray_t, rec)) return false;

        //Move intersection point forward by offset
        rec.p += offset;

        return true;
    }
    aabb bounding_box() const override
    {
        return bbox;
    }
private:
    shared_ptr<hittable> object;
    vec3 offset;
    aabb bbox;
};
class rotate_y : public hittable
{
public:
    rotate_y(shared_ptr<hittable> p, double angle) : object(p)
    {
        auto radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = object->bounding_box();

        //find the rotated bounding box by:
        /* for each point in the old bbox, rotate it, and then for each axis
         * store the min and max values reached by each rotated point.
         * then make a bbox out of those
         */
        point3 min(infinity, infinity, infinity);
        point3 max(-infinity, -infinity, -infinity);
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    //runs thru every combo of max and min on the three axes
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;
                    auto y = i*bbox.y.max + (1-i)*bbox.y.min;
                    auto z = i*bbox.z.max + (1-i)*bbox.z.min;

                    auto newx = cos_theta*x + sin_theta*z;
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3 new_point(newx, y, newz);
                    for (int a = 0; a < 3; a++){
                        min[a] = std::fmin(new_point[a], min[a]);
                        max[a] = std::fmax(new_point[a], max[a]);
                    }
                }
            }
        }
        bbox = aabb(min, max);
    }
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        //Transform ray from world space to obj space by reversing the transformation we did on the object
        //that is, rotating by -theta
        //rotation matrix for y is:
        /*
         * cos(theta) sin(theta)
         * -sin(theta) cos(theta)
         */ //to be multiplied with vector (x, z)
        /*
         * new x: cos(theta)x + sin(theta)z
         * new z: -sin(theta)x + cos(theta)z
         */
        //but here we want to rotate by -theta, cos(-theta) = cos(theta) and sin(-theta) = -sin(theta)
        auto origin = point3(
            cos_theta * r.origin().x() - sin_theta * r.origin().z(),
            r.origin().y(),
            sin_theta * r.origin().x() + cos_theta * r.origin().z()
        );
        auto direction = vec3(
            cos_theta * r.direction().x() - sin_theta * r.direction().z(),
            r.direction().y(),
            sin_theta * r.direction().x() + cos_theta * r.direction().z()
        );

        ray rotated_ray(origin, direction);

        //Determine if intersection occurs
        if (!object->hit(rotated_ray, ray_t, rec)) return false;

        //transform intersection from obj space back to world space by applying the rotation by theta
        rec.p = point3(
            cos_theta * rec.p.x() + sin_theta * rec.p.z(),
            rec.p.y(),
            -sin_theta * rec.p.x() + cos_theta * rec.p.z()
        );
        rec.normal = vec3(
            cos_theta * rec.normal.x() + sin_theta * rec.normal.z(),
            rec.normal.y(),
            -sin_theta * rec.normal.x() + cos_theta * rec.normal.z()
        );
        return true;
    }
    aabb bounding_box() const override
    {
        return bbox;
    }
private:
    shared_ptr<hittable> object;
    double sin_theta;
    double cos_theta;
    aabb bbox;
};



#endif //HITTABLE_H
