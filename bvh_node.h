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
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()){}
    bvh_node(std::vector<shared_ptr<hittable>>& objects, int start, int end)
    {
        //std::clog << "building node for " << start << " to " << end << std::endl;

        //Build the bounding box of the span of source objects
        bbox = aabb::empty;
        for (int object_index = start; object_index < end; object_index++)
        {
            bbox = aabb(bbox, objects[object_index]->bounding_box());
        }

        int axis = bbox.longest_axis(); //Find the longest axis of that bounding box

        //Sort objects along that axis
        auto comparator = (axis == 0) ? box_x_compare
                                  :(axis == 1) ? box_y_compare
                                               : box_z_compare;
        int object_span = end - start;

        if (object_span == 2)
        {
            left = objects[start];
            right = objects[start+1];
        }else if (object_span == 1)
        {
            left = objects[start];
            right = make_shared<hittable_list>();
        }else if (object_span <= 4)
        {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
            int mid = std::ceil((start + end)/ 2.0);
            left = make_shared<bvh_node>(objects, start, mid);
            right = make_shared<bvh_node>(objects, mid, end);
        }else
        {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);

            int num_buckets = 12;
            int optimal_partition = sah_partition(objects, start, end, axis, num_buckets, bbox);
            //std::clog << "optimal partition: " << optimal_partition << std::endl;

            if (optimal_partition == -1)
            {
                shared_ptr<hittable_list> list = make_shared<hittable_list>();
                for (int i = start; i < end; i++)
                {
                    list->add(objects[i]);
                }
                left = list;
                right = make_shared<hittable_list>();
            }else
            {
                left = make_shared<bvh_node>(objects, start, optimal_partition+1);
                right = make_shared<bvh_node>(objects, optimal_partition+1, end);
            }
        }
    }
    //returns a number in the range [0, num_buckets-2] for the best index to split after, or -1 if you should just make a leaf
    int sah_partition(const std::vector<shared_ptr<hittable>>& objects, int start, int end, int axis, int num_buckets, const aabb& parent_bbox) const
    {
        /*
         * for each obj from start -> end:
         *      determine which bucket it goes in using centroid of primitive's bounding box
         *      add 1 to the number of primitives in that bucket
         *      update the bounding box of that bucket
         */
        int primitive_count[num_buckets];
        aabb bucket_bounds[num_buckets];
        for (int i = 0; i < num_buckets; i++)
        {
            primitive_count[i] = 0;
            bucket_bounds[i] = aabb::empty;
        }
        const interval& axis_interval = bbox.axis_interval(axis);
        double bucket_length = axis_interval.size() / num_buckets;
        for (int i = start; i < end; i++)
        {
            double axis_coord = objects[i]->bounding_box().get_centroid()[axis];
            int bucket_ind = std::ceil((axis_coord - axis_interval.min) / bucket_length); //1 indexed!
            primitive_count[bucket_ind-1]++;
            bucket_bounds[bucket_ind-1] = aabb(bucket_bounds[bucket_ind-1], objects[i]->bounding_box());
        }

        int primitive_count_left_psums[num_buckets];
        aabb bucket_bounds_left_union[num_buckets];
        for (int i = 0; i < num_buckets; i++)
        {
            primitive_count_left_psums[i] = primitive_count[i];
            bucket_bounds_left_union[i] = bucket_bounds[i];
            if (i != 0)
            {
                primitive_count_left_psums[i]+=primitive_count_left_psums[i-1];
                bucket_bounds_left_union[i] = aabb(bucket_bounds_left_union[i], bucket_bounds_left_union[i-1]);
            }
        }
        int primitive_count_right_psums[num_buckets];
        aabb bucket_bounds_right_union[num_buckets];
        for (int i = num_buckets-1; i >= 0; i--)
        {
            primitive_count_right_psums[i] = primitive_count[i];
            bucket_bounds_right_union[i] = bucket_bounds[i];
            if (i != num_buckets-1)
            {
                primitive_count_right_psums[i]+=primitive_count_right_psums[i+1];
                bucket_bounds_right_union[i] = aabb(bucket_bounds_right_union[i], bucket_bounds_right_union[i+1]);
            }
        }

        int min_cost_index = 0; //best index to split AFTER
        double min_cost = infinity;
        for (int i = 0; i < num_buckets - 1; i++) //for loop through each bucket, but not the last one
        {
            int count_left = primitive_count_left_psums[i];
            double sa_left = bucket_bounds_left_union[i].surface_area();

            int count_right = primitive_count_right_psums[i+1];
            double sa_right = bucket_bounds_right_union[i+1].surface_area();

            if (count_left == 0){ sa_left = 0;}
            if (count_right == 0) {sa_right = 0;}

            //set cost_intersection = 1 and cost_bvh_traversal = 1/8
            /* std::clog << "sa left: " << sa_left << " count_left: " << count_left <<
                " sa_right: " << sa_right << " count right: " << count_right <<
                    " parent bbox sa: " << parent_bbox.surface_area() << std::endl; */
            double cost = 0.125 + (sa_left * count_left + sa_right * count_right) / parent_bbox.surface_area();
            //std::clog << "cost " << i << " is " << cost << std::endl;
            if (cost < min_cost)
            {
                min_cost = cost;
                min_cost_index = i;
            }
        }

        //compare with cost of making a leaf which is 1 * object_span
        if (end - start <= min_cost)
        {
            return -1;
        }
        //min_cost_index is in range [0, num_buckets-2]
        //std::string debug = "min cost index is " + std::to_string(min_cost_index);
        double optimal_bound = axis_interval.min + bucket_length * (min_cost_index+1);
        int i = start;
        for (; i < end; i++)
        {
            double axis_coord = objects[i]->bounding_box().get_centroid()[axis];

            /*debug+=" axis_coord of object " + std::to_string(i) + " is " + std::to_string(axis_coord) +
                " | optimal bound to stop at is " + std::to_string(optimal_bound) + "\n";*/

            if (axis_coord >= optimal_bound) break;
        }
        //std::clog << debug << std::endl;
        return std::max(start, i-1);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        if (!bbox.hit(r, ray_t)) return false;

        bool hit_left = left->hit(r, ray_t, rec);
        //if something got hit on the left, we see if there was smth on the right that would have been hit first
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t - 1e-8 : ray_t.max), rec);

        return hit_left || hit_right;
    }
    aabb bounding_box() const override {return bbox;}
    std::string to_string() const override
    {
        std::string s = "bvh node overall bounding box: [" + bbox.to_string() + "] \n";
        return s + "Left: \n" + left->to_string() + "\nRight: \n" + right->to_string();
    }
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
