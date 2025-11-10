//
// Created by Faye Yu on 11/9/25.
//

#ifndef BVH_NODE_H
#define BVH_NODE_H
#include "aabb.h"
#include "hittable_list.h"
#include <algorithm>

class bvh_node : public hittable
{
    public:
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, static_cast<long>(list.objects.size())){}
    bvh_node(std::vector<shared_ptr<hittable>>& objects, long start, long end)
    {
        //Build the bounding box of the span of source objects
        bbox = aabb::empty;
        for (long object_index=start; object_index<end; object_index++)
        {
            bbox = aabb(bbox, objects[object_index]->bounding_box());
        }

        int axis = bbox.longest_axis(); //Find the longest axis of that bounding box

        //Sort objects along that axis
        auto comparator = (axis == 0) ? box_x_compare
                                  :(axis == 1) ? box_y_compare
                                               : box_z_compare;
        size_t object_span = end - start;

        if (object_span == 3)
        {
            //TODO is this expensive?? idk ask ai
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
            left = make_shared<bvh_node>(objects, start, start+2);
            right = objects[end-1]; //is a primitive
        }else if (object_span == 2)
        {
            left = objects[start];
            right = objects[start+1];
        }else if (object_span == 1)
        {
            left = right = objects[start];
        }else
        {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);

            //Split the list: if the size of list is < 12, split into that number of buckets
            //Otherwise split into 12 buckets
            int num_buckets = end-start < 12 ? static_cast<int>(end - start) : 12;
            int optimal_partition = sah_partition(objects, start, end, axis, num_buckets, (double)bbox.axis_interval(axis).size() / num_buckets);
            left = make_shared<bvh_node>(objects, start, start+optimal_partition+1);
            right = make_shared<bvh_node>(objects, start+optimal_partition+1, end);
        }
    }
    int sah_partition(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end, int axis, int num_buckets, double bucket_length) const
    {
        int best_index = 0;
        int current_index = 0;
        aabb left = objects[current_index]->bounding_box();
        double min_sa = infinity;
        for (int i = 1; i <= num_buckets; i++) //for loop through each bucket
        {
            double bound = bucket_length*i;
            //loop through bounding boxes, adding them to left box until we'd go into another bucket
            while (objects[current_index]->bounding_box().get_centroid()[axis] <= bound && current_index < end)
            {
                left = aabb(left, objects[current_index]->bounding_box());
                current_index++;
            }
            if (current_index == end) break;

            double left_sa = left.surface_area(); //calc the surface area of left and right, sum them, save index
            if (left_sa > min_sa) continue; //little optimization: if the left box SA is > the min SA sum already, don't calc the right one

            aabb right = aabb::empty;
            //calc the right box TODO ask ai if computationally expensive??
            for (int j = current_index+1; j < end; j++)
            {
                right = aabb(right, objects[j]->bounding_box());
            }
            double right_sa = right.surface_area();
            if (left_sa + right_sa < min_sa) //at the end of loop: the partition w the minimum SA is what we choose
            {
                best_index = current_index == 0 ? 0 : current_index-1;
                min_sa = left_sa + right_sa;
            }
        }
        //returns the last index to include in the left box
        return best_index;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        if (!bbox.hit(r, ray_t)) return false;

        bool hit_left = left->hit(r, ray_t, rec);
        //if something got hit on the left, we see if there was smth on the right that would have been hit first
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }
    aabb bounding_box() const override {return bbox;};
private:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;

    static bool box_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b, int axis_index){
        auto a_axis_int = a->bounding_box().axis_interval(axis_index);
        auto b_axis_int = b->bounding_box().axis_interval(axis_index);
        return a_axis_int.min < b_axis_int.min;
    }
    static bool box_x_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b)
    {
        return box_compare(a, b, 0);
    }
    static bool box_y_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b)
    {
        return box_compare(a, b, 1);
    }
    static bool box_z_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b)
    {
        return box_compare(a, b,2);
    }
};

#endif //BVH_NODE_H
