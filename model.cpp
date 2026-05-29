#include "model.hpp"

#include <sstream>
#include <fstream>
#include <format>

Model::Model(const std::string& filename){
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << std::format("open file %s failed.\n", filename);
        return;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue; 

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") {
            Vector3 v;
            iss >> v[0] >> v[1] >> v[2];
            verts.push_back(v);
        } else if (token == "f") {
            int v, vt, vn;
            char trash;
            while (iss >> v >> trash >> vt >> trash >> vn) {
                facet_vrt.push_back(--v);
            }
        }
    }
}

Vector3 Model::vert(const int i) const {
    return verts[i];
}

Vector3 Model::vert(const int iface, const int nth) const {
    return verts[facet_vrt[3 * iface + nth]];
}

int Model::nverts() const {
    return verts.size();
}

int Model::nfaces() const {
    return facet_vrt.size() / 3;
}