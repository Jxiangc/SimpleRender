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
        char trash; // 用于丢弃斜杠
        iss >> token;

        if (token == "v") {
            Vector3 v;
            iss >> v[0] >> v[1] >> v[2];
            verts.push_back(v);
        } else if (token == "vn") {
            Vector3 vn;
            iss >> vn[0] >> vn[1] >> vn[2];
            norms.push_back(vn);
        } else if (token == "vt") {
            Vector2 vt;
            iss >> vt[0] >> vt[1] >> trash;
            tex.push_back(vt);
        } else if (token == "f") {
            int v, vt, vn;
            while (iss >> v >> trash >> vt >> trash >> vn) {
                facet_vrt.push_back(--v);
                facet_nrm.push_back(--vn);
                facet_tex.push_back(--vt);
            }
        }
    }

    auto load_texture = [&](const std::string& suffix, TGAImage& image) {
        std::filesystem::path tex_path = std::filesystem::path(filename).replace_extension("").string() + suffix;
        image.read_tga_file(tex_path.string());
    };
    load_texture("_nm_tangent.tga", normal_map);
    load_texture("_diffuse.tga", diffuse_map);
    load_texture("_spec.tga", specular_map);
}

Vector3 Model::vert(const int i) const {
    return verts[i];
}

Vector3 Model::vert(const int iface, const int nth) const {
    return verts[facet_vrt[3 * iface + nth]];
}

Vector3 Model::norm(const int i) const {
    return norms[i];
}

Vector3 Model::norm(const int iface, const int nth) const {
    return norms[facet_nrm[3 * iface + nth]];
}

Vector2 Model::uv(const int i) const {
    return tex[i];
}

Vector2 Model::uv(const int iface, const int nth) const {
    return tex[facet_tex[3 * iface + nth]];
}

int Model::nverts() const {
    return verts.size();
}

int Model::nfaces() const {
    return facet_vrt.size() / 3;
}