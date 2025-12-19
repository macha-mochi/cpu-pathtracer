cmake_minimum_required(VERSION 3.31)
project(raytracing)

set(CMAKE_CXX_STANDARD 20)

add_executable(raytracing main.cpp
        vec3.h
        color.h
        ray.h
        hittable.h
        sphere.h
        hittable_list.h
        rtweekend.h
        interval.h
        camera.h
        material.h
        aabb.h
        bvh_node.h
        quad.h
        triangle_mesh.h
        obj_loader.h
        triangle.h
        bxdf.h)
