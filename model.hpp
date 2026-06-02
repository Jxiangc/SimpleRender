#pragma once
#include <vector>
#include <string>
#include <filesystem>

#include "tgaimage.hpp"
#include "linearalgebra.hpp"

class Model{
public:
    Model(const std::string& filename);
    Model() = delete;

    int nverts() const;
    int nfaces() const;

    Vector3 vert(const int i) const;
    Vector3 vert(const int iface, const int nth) const;
    Vector3 norm(const int i) const;
    Vector3 norm(const int iface, const int nth) const;
    Vector2 uv(const int i) const;
    Vector2 uv(const int iface, const int nth) const;

    const TGAImage& normal() const { return normal_map; }
    const TGAImage& diffuse() const { return diffuse_map; }
    const TGAImage& specular() const { return specular_map; }

private:
    std::vector<Vector3> verts = {};
    std::vector<Vector3> norms = {};
    std::vector<Vector2> tex = {};
    std::vector<int> facet_vrt = {};
    std::vector<int> facet_nrm = {};
    std::vector<int> facet_tex = {};

    TGAImage normal_map = {};
    TGAImage diffuse_map = {};
    TGAImage specular_map = {};
};