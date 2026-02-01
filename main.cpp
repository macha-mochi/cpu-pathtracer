#include <chrono>
#include "rtweekend.h"

#include "bvh_node.h"
#include "camera.h"
#include "hittable_list.h"
#include "light.h"
#include "obj_loader.h"
#include "sphere.h"
#include "quad.h"

void make_big_scene()
{
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(Metal::steel, albedo, fuzz, 0);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5, 0);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5, 0);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    //auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    auto material3 = make_shared<metal>(Metal::silver);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 1200;
    cam.samples_per_pixel = 10;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.6;
    cam.focus_dist    = 10.0;

    cam.render(world); //camera will call initialize() at beginning of render()
}
void make_small_test_scene()
{
    hittable_list world;

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    //auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_center = make_shared<metal>(Metal::copper);
    //auto material_left = make_shared<metal>(Metal::steel);
    auto material_left = make_shared<dielectric>(1.50, 0);
    auto material_bubble = make_shared<dielectric>(1.00 / 1.50, 0);
    //auto material_right = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
    auto material_right = make_shared<metal>(Metal::gold);

    //ground
    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.2), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.4, material_bubble));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 90;
    cam.lookfrom = point3(0,0,0);
    cam.lookat   = point3(0,0,-1);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0; //10.0;
    //cam.focus_dist    = 3.4;
    cam.russian_roulette_termination = true;

    cam.render(world);
}
void quads()
{
    hittable_list world;

    //materials
    auto left_red = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal = make_shared<lambertian>(color(0.2, 0.8, 0.8));
    auto glass_ball = make_shared<dielectric>(1.5, 0);

    //quads
    world.add(make_shared<quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), left_red));
    world.add(make_shared<quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<quad>(point3( 3,-2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<quad>(point3(-2,-3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal));
    world.add(make_shared<sphere>(point3(0, 0, 6), 1, glass_ball));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 80;
    cam.lookfrom = point3(0,0,9);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
    cam.russian_roulette_termination = false;

    cam.render(world);
}
void load_dragon()
{
    hittable_list world;

    obj_loader loader = obj_loader("/Users/fayeyu/CLionProjects/raytracing/objs");
    /*auto mat = make_shared<lambertian>(color(1.0, 0.0, 0.0));
    world.add(make_shared<sphere>(point3(0, 2, 0), 1, mat));*/

    auto mat1 = make_shared<lambertian>(color(0.2, 0.5, 0.7));
    shared_ptr<triangle_mesh> mesh1 = loader.load("dragon.obj", mat1);

    auto bvh = make_shared<bvh_node>(mesh1->triangles);


    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 512;
    cam.samples_per_pixel = 8;
    cam.max_depth         = 3;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 10;
    cam.lookfrom = point3(0.5,0.07,-1);
    cam.lookat   = point3(0,0.02,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.0;
    cam.focus_dist    = 3;

    cam.render(world);

    std::clog << "triangles: " << mesh1->triangles.objects.size() << std::endl;
}
void triangle_test(){
    hittable_list world;

    auto mat1 = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<triangle>(vec3(0, 0, -1), vec3(1, 0, -1), vec3(0, 1, -1), mat1));
    //world.add(make_shared<sphere>(vec3(0, 0, -1), 0.5, mat1));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 1;
    cam.max_depth         = 30;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 90;
    cam.lookfrom = point3(-2,2,1);
    cam.lookat   = point3(0,0,-1);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.0;
    cam.focus_dist    = 3;

    cam.render(world);
}
void simple_light()
{
    hittable_list world;

    auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_mat));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, ground_mat));

    auto intensity = 8;
    auto difflight = make_shared<diffuse_light>(8*color(1.0, 0, 0.5), color(1.0, 1.0, 1.0));
    world.add(make_shared<quad>(point3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0), difflight));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 20;
    cam.lookfrom = point3(26,3,6);
    cam.lookat   = point3(0,2,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.render(world);
}
void cornell_box() {
    hittable_list world;
    hittable_list lights;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light_mat = make_shared<diffuse_light>(15 * color(1.0, 0.9, 0.7), color(1.0, 1.0, 1.0));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    auto small_l = make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light_mat);
    auto ql = make_shared<quad_light>(small_l, light_mat);
    world.add(ql);
    lights.add(ql);

    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    /*shared_ptr<hittable> bigger_box = box(point3(0,0,0), point3(165,330,165), white);
    bigger_box = make_shared<rotate_y>(bigger_box, -15);
    bigger_box = make_shared<translate>(bigger_box, vec3(130,0,265));
    world.add(bigger_box);

    shared_ptr<hittable> smaller_box = box(point3(0,0,0), point3(165,165,165), white);
    smaller_box = make_shared<rotate_y>(smaller_box, 18);
    smaller_box = make_shared<translate>(smaller_box, vec3( 265,0,130));
    world.add(smaller_box);*/

    auto metal_mat = make_shared<metal>(Metal::silver, color(1, 1, 1), 0.5, 0);
    auto glass = make_shared<dielectric>(1.5, 0.4);
    world.add(make_shared<sphere>(point3(200, 300, 300), 70, metal_mat));
    world.add(make_shared<sphere>(point3(400, 300, 300), 70, glass));

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 256;
    cam.samples_per_pixel = 8;
    cam.max_depth         = 16;
    cam.background = color(0, 0, 0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);
    cam.flipHorizontal = true;

    cam.defocus_angle = 0;
    cam.russian_roulette_termination = true;

    //std::cout << world.to_string() << std::endl;
    cam.render(world, lights);
}
void cornell_box_metals() {
    hittable_list world;
    hittable_list lights;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light_mat = make_shared<diffuse_light>(15 * color(1.0, 0.9, 0.7), color(1.0, 1.0, 1.0));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    auto small_l = make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light_mat);
    auto ql = make_shared<quad_light>(small_l, light_mat);
    world.add(ql);
    lights.add(ql);

    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    auto steel = make_shared<metal>(Metal::steel, color(1, 1, 1), 0, 0);
    auto copper = make_shared<metal>(Metal::copper, color(1, 1, 1), 0, 0);
    auto silver = make_shared<metal>(Metal::silver, color(1, 1, 1), 0, 0);
    auto gold = make_shared<metal>(Metal::gold, color(1, 1, 1), 0, 0);

    world.add(make_shared<sphere>(point3(100, 80, 400), 80, steel));
    world.add(make_shared<sphere>(point3(200, 50, 150), 50, copper));
    world.add(make_shared<sphere>(point3(310, 60, 220), 60, silver));
    world.add(make_shared<sphere>(point3(410, 120, 400), 120, gold));

    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 1024;
    cam.samples_per_pixel = 1024;
    cam.max_depth         = 16;
    cam.background = color(0, 0, 0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);
    cam.flipHorizontal = true;

    cam.defocus_angle = 0;
    cam.russian_roulette_termination = true;

    //std::cout << world.to_string() << std::endl;
    cam.render(world, lights);
}
void spider_lily_obj(){
    hittable_list world;
    hittable_list lights;

    obj_loader loader = obj_loader("/Users/fayeyu/CLionProjects/raytracing/objs");
    auto mat1 = make_shared<lambertian>(color(1.0, 0.2, 0.5));
    shared_ptr<triangle_mesh> mesh1 = loader.load("fire.obj", mat1);
    hittable_list tris = mesh1->triangles;
    world.add(make_shared<translate>(make_shared<bvh_node>(tris), vec3(278, 278, 278)));

    auto light_mat = make_shared<diffuse_light>(10 * color(1.0, 0.9, 0.7), color(1.0, 1.0, 1.0));
    auto small_l = make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light_mat);
    auto ql = make_shared<quad_light>(small_l, light_mat);
    world.add(ql);
    lights.add(ql);

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 256;
    cam.samples_per_pixel = 8;
    cam.max_depth         = 8;
    cam.background = color(0.3, 0.3, 0.6);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);
    cam.flipHorizontal = true;

    cam.defocus_angle = 0;
    cam.russian_roulette_termination = true;

    cam.render(world, lights);

    std::clog << "triangles: " << tris.objects.size() << std::endl;
}
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main() {
    auto start = std::chrono::high_resolution_clock::now();

    switch (9)
    {
        case 1: make_big_scene(); break;
        case 2: make_small_test_scene(); break;
        case 3: quads(); break;
        case 4: triangle_test(); break;
        case 5: simple_light(); break;
        case 6: load_dragon(); break;
        case 7: spider_lily_obj();
        case 8: cornell_box(); break;
        case 9: cornell_box_metals(); break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::clog << "Time: " << duration.count() << " ms" << std::endl;
}