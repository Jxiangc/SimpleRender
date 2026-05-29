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

private:
    std::vector<Vector3> verts = {};
    std::vector<int> facet_vrt = {};
};