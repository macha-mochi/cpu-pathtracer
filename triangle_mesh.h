//
// Created by Faye Yu on 11/16/25.
//

#ifndef MESH_H
#define MESH_H

#include <utility>

#include "hittable.h"
#include "hittable_list.h"

class triangle_mesh : public hittable
{
/*
 * remember you need mesh to also be a bvh node. this is done in main.cpp
 */
public:
    hittable_list triangles;
    triangle_mesh(hittable_list tris, std::vector<point3> v, std::vector<point3> vt,
        std::vector<point3> vn, const shared_ptr<material>& mat) : triangles(std::move(tris)),
    v(std::move(v)), vt(std::move(vt)), vn(std::move(vn))
    {}
    //we copy these variables to prevent them from becoming dangling refs
    //ex: v, vt, vn are created by obj_loader, mat is shared ptr
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        bool b = triangles.hit(r, ray_t, rec);
        return b;
    }

    aabb bounding_box() const override {return triangles.bounding_box();}
private:
    //const int nTriangles;
    //const int nVertices;
    std::vector<point3> v;
    std::vector<point3> vt;
    std::vector<point3> vn;
};

#endif //MESH_H
