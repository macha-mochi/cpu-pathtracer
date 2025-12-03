//
// Created by Faye Yu on 11/16/25.
//

#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H
#include <iostream>
#include <fstream>
#include <filesystem>
#include <utility>
#include "triangle_mesh.h"
#include "material.h"
#include "triangle.h"

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

class obj_loader
{
public:
    std::vector<std::string> obj_filepaths;
    obj_loader(const std::string& obj_folder_path)
    {
        //detect every .obj in the folder, store their (valid) paths in an array
        for (auto const& entry : fs::directory_iterator(obj_folder_path))
        {
            if (entry.is_regular_file() && entry.path().extension().string() == ".obj")
            {
                obj_filepaths.push_back(entry.path().string());
            }
        }
    }

    /**
     * @param obj_name
     * @param mat the material to make this mesh
     * @return a shared_ptr to the triangle_mesh in the .obj file with name obj_name,
     * and nullptr if no .obj file found of that name
     */
    shared_ptr<triangle_mesh> load(const std::string& obj_name, const shared_ptr<material>& mat)
    {
        //find the obj with this name in our array
        std::string path;
        for (const std::string& s : obj_filepaths)
        {
            if (s.compare(s.length() - obj_name.length(), obj_name.length(), obj_name) == 0)
            {
                path = s;
            }
        }
        if (path.empty())
        {
            std::clog << "Could not find OBJ of that name, returning nullptr" << std::endl;
            return nullptr;
        }

        std::string line;

        hittable_list mesh_faces;
        std::vector<point3> global_v;
        std::vector<point3> global_vt;
        std::vector<point3> global_vn;

        std::ifstream read_obj(path);
        std::clog << "reading: " << path << std::endl;
        while (getline(read_obj, line))
        {
            auto tokens = tokenize(line, " ");
            const std::string& first_token = tokens[0];
            if (first_token == "v")
            {
                //read to global_v
                double x = std::stod(tokens[1]);
                double y = std::stod(tokens[2]);
                double z = std::stod(tokens[3]);
                global_v.emplace_back(x, y, z);
            }
            else if (first_token == "vt")
            {
                //read to global_vt
                double x = std::stod(tokens[1]);
                double y = std::stod(tokens[2]);
                double z = 0;
                global_vt.emplace_back(x, y, z);
            }
            else if (first_token == "vn")
            {
                //read to global_vn
                double x = std::stod(tokens[1]);
                double y = std::stod(tokens[2]);
                double z = std::stod(tokens[3]);
                global_vn.emplace_back(x, y, z);
            }
            else if (first_token == "f")
            {
                std::vector<int> vertex_indices;
                for (int j = 1; j <= 3; j++)
                {
                    std::vector<std::string> indices = tokenize(tokens[j], "/");
                    //TODO we currently don't care abt texture coords or vertex normals
                    vertex_indices.push_back(std::stoi(indices[0]));
                }
                size_t v0 = vertex_indices[0]-1;
                size_t v1 = vertex_indices[1]-1;
                size_t v2 = vertex_indices[2]-1;
                auto tri = make_shared<triangle>(global_v[v0], global_v[v1],
                                                 global_v[v2], mat);
                //std::clog << "tri: " << vertex_indices[0] << ", " << vertex_indices[1] << ", " << vertex_indices[2] << std::endl;
                //std::clog << tri->to_string() << std::endl;
                mesh_faces.add(tri);

                if (tokens.size() == 5) //we have a quad face to triangulate
                {
                    std::vector<std::string> indices = tokenize(tokens[4], "/");
                    vertex_indices.push_back(std::stoi(indices[0]));
                    size_t v3 = vertex_indices[3]-1;
                    auto tri1 = make_shared<triangle>(global_v[v0], global_v[v2],
                                                      global_v[v3], mat);
                    mesh_faces.add(tri1);
                    //std::clog << "tri1: " << vertex_indices[0] << ", " << vertex_indices[2] << ", " << vertex_indices[3] << std::endl;
                    //std::clog << tri1->to_string() << std::endl;
                }
            }
        }
        shared_ptr<triangle_mesh> mesh = make_shared<triangle_mesh>(mesh_faces, global_v, global_vn, global_vt,
                                                                    mat);
        return mesh;
    }
private:
    /**
     *
     * @param line the string to parse
     * @return array of tokens (strings)
     */
    static std::vector<std::string> tokenize(std::string& line, const std::string& delim)
    {
        //input code modified from stack overflow: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c

        std::vector<std::string> result;
        size_t pos = line.find(delim);
        do
        {
            result.push_back(line.substr(0, pos));
            line.erase(0, pos + delim.length());
            pos = line.find(delim);
        }while (pos != std::string::npos);
        if (!line.empty()) result.push_back(line);

        return result;
    }
};

#endif //OBJ_LOADER_H
