#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>

#include "simplegl.hpp"
#include "tgaimage.hpp"
#include "linearalgebra.hpp"
#include "model.hpp"

extern Matrix4 ModelView, Viewport, Projection;
extern std::vector<double> zbuffer;

class SimpleShader : public IShader {
public:
    SimpleShader() = delete;
    SimpleShader(const Model& m): model(m) {}
    SimpleShader(const Model& m, TGAColor c): model(m), color(c) {}

    virtual Vector4 vertex(const int iface, const int nth) const {
        Vector3 v = model.vert(iface, nth);
        Vector4 gl_Vertex = ModelView.multiply(Vector4{v[0], v[1], v[2], 1});
        return Projection.multiply(gl_Vertex);
    }

    virtual std::pair<bool, TGAColor> fragment(const Vector3& bar) const override {
        return {false, color};
    }

    const Model& model;
    TGAColor color = {};
};

TGAColor get_random_color(){
    uint8_t b = static_cast<uint8_t>(rand() % 255);
    uint8_t g = static_cast<uint8_t>(rand() % 255);
    uint8_t r = static_cast<uint8_t>(rand() % 255);
    uint8_t a = static_cast<uint8_t>(rand() % 255);
    return {b, g, r, a};
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <obj_file_path>\n";
        return 1;
    }
    
    constexpr int width  = 1024;
    constexpr int height = 1024;

    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    Vector3 eye{-1, 0, 2}, center{0, 0, 0}, up{0, 1, 0};

    lookat(eye, center, up);
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_perspective(PI / 3, width / static_cast<double>(height), 1, 10);
    init_zbuffer(width, height);

    srand(time(0));
    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);
        SimpleShader shader(model);
        for (int i = 0; i < model.nfaces(); i++) {
            shader.color = get_random_color();
            Triangle clip = {shader.vertex(i, 0), shader.vertex(i, 1), shader.vertex(i, 2)};
            rasterize(clip, shader, framebuffer);
        }
    }

    std::string output_filename = "framebuffer.tga";
    std::filesystem::path name = std::filesystem::path(argv[1]).stem();
    output_filename = name.string() + ".tga";
    framebuffer.write_tga_file(output_filename);

    return 0;
}
