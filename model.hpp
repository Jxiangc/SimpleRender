#pragma once
#include <vector>
#include <string>

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
    Vector2 UVcoord(const int i) const;
    Vector2 UVcoord(const int iface, const int nth) const;

private:
    std::vector<Vector3> verts = {};
    std::vector<Vector3> norms = {};
    std::vector<Vector2> uv = {};
    std::vector<int> facet_vrt = {};
    std::vector<int> facet_nrm = {};
    std::vector<int> facet_uv = {};
};