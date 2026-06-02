#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
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

Vector3 RGB2normal(const TGAColor& color) { // [0, 255] -> [-1, 1]
    return Vector3{
        2.0 * color[2] / 255 - 1, // R -> X
        2.0 * color[1] / 255 - 1, // G -> Y
        2.0 * color[0] / 255 - 1  // B -> Z
    };
}

class SimpleShader : public IShader {
public:
    SimpleShader() = delete;
    SimpleShader(const Model& m, const Vector3& light_dir): model(m) {
        l = ModelView.multiply(Vector4{light_dir[0], light_dir[1], light_dir[2], 0}).xyz().normalized();
        v = {0, 0, 1};
    }

    virtual Vector4 vertex(const int iface, const int nth) {
        Vector3 v = model.vert(iface, nth);
        tri[nth] = Vector4{v[0], v[1], v[2], 1};
        nrm[nth] = model.norm(iface, nth);
        uv[nth] = model.uv(iface, nth);
        return Projection.multiply(ModelView.multiply(tri[nth])); // 返回裁剪空间坐标
    }

    virtual std::pair<bool, TGAColor> fragment(const Vector3& bar) const override {
        Vector3 h = (v + l).normalized();
        Vector2 curr_uv = uv[0] * bar[0] + uv[1] * bar[1] + uv[2] * bar[2];
        
        auto get_texture_color = [&](const TGAImage& image) {
            int u = std::clamp(static_cast<int>(curr_uv[0] * image.width()), 0, image.width() - 1);
            int v = std::clamp(static_cast<int>((1.0 - curr_uv[1]) * image.height()), 0, image.height() - 1);
            return image.get(u, v);
        };

        Matrix<2, 2> UV = {uv[1]- uv[0], uv[2] - uv[0]};
        Vector3 n_model = (nrm[0] * bar[0] + nrm[1] * bar[1] + nrm[2] * bar[2]).normalized();
        Vector3 n_tangent = RGB2normal(get_texture_color(model.normal()));
        Vector3 n;

        if (std::abs(UV.det()) < 1e-8) {
            n = NormalMatrix.multiply(n_model).normalized();
        } else {
            Matrix<3, 2> Edge = {tri[1].xyz() - tri[0].xyz(), tri[2].xyz() - tri[0].xyz()};
            Matrix<3, 2> TB = Edge.multiply(UV.inverse());
            Vector3 t = TB[0], b = TB[1];
            Matrix3 TBN = {t, b, n_model};
            n = NormalMatrix.multiply(TBN.multiply(n_tangent)).normalized();
        }

        TGAColor diff = get_texture_color(model.diffuse());
        TGAColor spec = get_texture_color(model.specular());

        if (diff.bytespp == 4 && diff[3] < 255) return {true, {}}; 

        double ambient = intensity * Ka;
        double diffuse = intensity * Kd * std::max(0.0, n.dot(l));
        double specular = intensity * Ks * std::pow(std::max(0.0, n.dot(h)), shininess);

        TGAColor c;
        for (int i = 0; i < 3; i++) {
            c[i] = static_cast<int>(std::min(255.0, (ambient + diffuse) * diff[i] + specular * spec[2]));
        }
        return {false, c};
    }

    const Model& model;
    Triangle tri;
    Normal nrm;
    UVcoords uv;
    Vector3 l;
    Vector3 v;
    double intensity = 1;
    double Ka = 0.4, Kd = 1.0, Ks = 0.4;
    double shininess = 32;
    TGAColor color = {};
};

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << std::format("Usage: %s <model.obj> [<nm.tga>]\n", argv[0]);
        return 1;
    }

    srand(time(0));

    constexpr int width  = 1024;
    constexpr int height = 1024;

    TGAImage framebuffer(width, height, TGAImage::RGB);
    Vector3 eye{-1, 0, 2}, center{0, 0, 0}, up{0, 1, 0}, light{1, 2, 2};

    lookat(eye, center, up);
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_perspective(PI / 3, width / static_cast<double>(height), 1, 10);
    init_zbuffer(width, height);

    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);
        SimpleShader shader(model, light - center);
        for (int i = 0; i < model.nfaces(); i++) {
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
