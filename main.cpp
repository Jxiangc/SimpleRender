#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>

#include "simplegl.hpp"
#include "tgaimage.hpp"
#include "linearalgebra.hpp"
#include "model.hpp"

extern Matrix4 ModelView, Viewport, Projection;
extern Matrix3 NormalMatrix;
extern std::vector<double> zbuffer;

TGAColor get_random_color(){
    uint8_t b = static_cast<uint8_t>(rand() % 255);
    uint8_t g = static_cast<uint8_t>(rand() % 255);
    uint8_t r = static_cast<uint8_t>(rand() % 255);
    uint8_t a = static_cast<uint8_t>(rand() % 255);
    return {b, g, r, a};
}

Vector3 RGB2normal(const TGAColor& color) {
    return Vector3{
        2.0 * color[2] / 255 - 1, // R -> X
        2.0 * color[1] / 255 - 1, // G -> Y
        2.0 * color[0] / 255 - 1  // B -> Z
    };
}

class SimpleShader : public IShader {
public:
    SimpleShader() = delete;
    SimpleShader(const Model& m, const TGAImage& n, const Vector3& eye_dir, const Vector3& light_dir): model(m), nmap(n) {
        l = light_dir.normalized();
        v = eye_dir.normalized();
    }

    virtual Vector4 vertex(const int iface, const int nth) const {
        Vector3 v = model.vert(iface, nth);
        Vector4 gl_Vertex = ModelView.multiply(Vector4{v[0], v[1], v[2], 1});
        return Projection.multiply(gl_Vertex);
    }

    virtual Vector3 normal(const int iface, const int nth) const {
        return NormalMatrix.multiply(model.norm(iface, nth));
    }

    virtual Vector2 uv(const int iface, const int nth) const {
        return model.UVcoord(iface, nth);
    }

    virtual std::pair<bool, TGAColor> fragment(const Vector3& bar) const override {
        return {false, color};
    }

    virtual std::pair<bool, TGAColor> fragment(const Vector3& bar, const UVcoords& uv) const override {
        Vector3 h = (v + l).normalized();

        Vector2 curr_uv = uv[0] * bar[0] + uv[1] * bar[1] + uv[2] * bar[2];
        int tx = std::clamp(static_cast<int>(curr_uv[0] * nmap.width()), 0, nmap.width() - 1);
        int ty = std::clamp(static_cast<int>((1.0 - curr_uv[1]) * nmap.height()), 0, nmap.height() - 1); // 注意TGA的UV坐标系与屏幕坐标系的y轴方向相反，因此需要用1.0减去v坐标
        Vector3 n = RGB2normal(nmap.get(tx, ty)).normalized();

        double ambient = intensity * Ka;
        double diffuse = intensity * Kd * std::max(0.0, n.dot(l));
        double specular = intensity * Ks * std::pow(std::max(0.0, n.dot(h)), shininess);
        double intensity_sum = ambient + diffuse + specular;

        return {false, TGAColor{
            static_cast<uint8_t>(std::min(1.0, intensity_sum) * 255),
            static_cast<uint8_t>(std::min(1.0, intensity_sum) * 255),
            static_cast<uint8_t>(std::min(1.0, intensity_sum) * 255),
            255
        }};
    }

    const Model& model;
    const TGAImage& nmap;
    Vector3 l;
    Vector3 v;
    double intensity = 1;
    double Ka = 0.1, Kd = 0.8, Ks = 0.4;
    double shininess = 16;
    TGAColor color = {};
};

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << std::format("Usage: %s <model.obj> [<nm.tga>]\n", argv[0]);
        return 1;
    }

    srand(time(0));

    std::vector<std::string> model_file_path;
    std::vector<std::string> nmap_file_path;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.ends_with(".obj")) model_file_path.push_back(arg);
        else if (arg.ends_with(".tga")) nmap_file_path.push_back(arg);
    }

    constexpr int width  = 1024;
    constexpr int height = 1024;

    TGAImage framebuffer(width, height, TGAImage::RGB);
    Vector3 eye{-1, 0, 2}, center{0, 0, 0}, up{0, 1, 0}, light{1, 2, 2};

    lookat(eye, center, up);
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_perspective(PI / 3, width / static_cast<double>(height), 1, 10);
    init_zbuffer(width, height);

    for (int m = 0; m < model_file_path.size(); m++) {
        Model model(model_file_path[m]);
        TGAImage nmap; nmap.read_tga_file(nmap_file_path[m]);
        SimpleShader shader(model, nmap, eye - center, light - center);
        for (int i = 0; i < model.nfaces(); i++) {
            Triangle clip;
            Normal norms;
            UVcoords uv;
            for (int j = 0; j < 3; j++) {
                clip[j] = shader.vertex(i, j);
                norms[j] = shader.normal(i, j);
                uv[j] = shader.uv(i, j);
            }
            rasterize(clip, norms, uv, shader, framebuffer);
        }
    }

    std::string output_filename = "framebuffer.tga";
    std::filesystem::path name = std::filesystem::path(argv[1]).stem();
    output_filename = name.string() + ".tga";
    framebuffer.write_tga_file(output_filename);

    return 0;
}
