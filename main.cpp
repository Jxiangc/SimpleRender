#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>

#include "tgaimage.hpp"
#include "linearalgebra.hpp"
#include "model.hpp"

#define PI 3.14159265358979323846
#define TAU (2 * PI)

enum Axis {X, Y, Z};

constexpr TGAColor white  = { 255, 255, 255, 255 }; // attention, BGRA order
constexpr TGAColor green  = {   0, 255,   0, 255 };
constexpr TGAColor red    = {   0,   0, 255, 255 };
constexpr TGAColor blue   = { 255,   0,   0, 255 };
constexpr TGAColor yellow = {   0, 200, 255, 255 };

Matrix4 ModelView, Viewport, Projection;

TGAColor get_random_color(){
    uint8_t b = static_cast<uint8_t>(rand() % 255);
    uint8_t g = static_cast<uint8_t>(rand() % 255);
    uint8_t r = static_cast<uint8_t>(rand() % 255);
    uint8_t a = static_cast<uint8_t>(rand() % 255);
    return {b, g, r, a};
}

Vector3 rot(Vector3 v, Axis axis, double angle){
    Matrix4 R;
    double c = std::cos(angle), s = std::sin(angle);
    if (axis == X) {
        R << 1, 0, 0, 0,
             0, c, -s, 0,
             0, s, c, 0,
             0, 0, 0, 1;
    } else if (axis == Y) {
        R << c, 0, s, 0,
             0, 1, 0, 0,
             -s, 0, c, 0,
             0, 0, 0, 1;
    } else if (axis == Z) {
        R << c, -s, 0, 0,
             s, c, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1;
    } else {
        std::cerr << "unsupported rotation axis\n";
        return v;
    }
    Vector4 v4 = R.multiply(Vector4{v[0], v[1], v[2], 1});
    return {v4[0], v4[1], v4[2]};
}

void bresenhamLine(int ax, int ay, int bx, int by, TGAImage& framebuffer, TGAColor color){
    bool steep = std::abs(by - ay) > std::abs(bx - ax);
    if (steep) {
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    int dx = bx - ax, dy = by - ay;
    int stepX = dx >= 0 ? 1 : -1;
    int stepY = dy >= 0 ? 1 : -1;
    dy = std::abs(dy);
    dx = std::abs(dx);

    int y = ay, pk = (dy << 1) - dx;
    for (int x = ax; x != bx; x += stepX) {
        steep ? framebuffer.set(y, x, color) : framebuffer.set(x, y, color);
        if (pk >= 0) {
            y += stepY;
            pk -= (dx << 1);
        }
        pk += (dy << 1);
    }
}

void LookAt(Vector3 eye, Vector3 center, Vector3 up) {
    Vector3 z = (eye - center).normalized();
    Vector3 x = up.cross(z).normalized();
    Vector3 y = z.cross(x).normalized(); // 重新计算y以确保正交性

    Matrix4 Model;
    Model << 1, 0, 0, -eye[0],
             0, 1, 0, -eye[1],
             0, 0, 1, -eye[2],
             0, 0, 0, 1;
    
    Matrix4 View; // 视图矩阵为正交矩阵，故其逆矩阵等于其转置矩阵
    View << x[0], x[1], x[2], 0,
            y[0], y[1], y[2], 0,
            z[0], z[1], z[2], 0,
            0, 0, 0, 1;

    ModelView = View.multiply(Model);
}

void perspective(double fovY, double aspect, double near, double far){
    double f = 1.0 / std::tan(fovY / 2);
    Projection << f / aspect, 0, 0, 0,
                  0, f, 0, 0,
                  0, 0, (far + near) / (near - far), (2 * far * near) / (near - far),
                  0, 0, -1, 0;
}

void viewport(int X, int Y, int w, int h){
    // X, Y 是视口左下角的坐标，w, h 是视口的宽和高
    Viewport << w / 2.0, 0, 0, X + w / 2.0,
                0, h / 2.0, 0, Y + h / 2.0,
                0, 0, 1, 0,
                0, 0, 0, 1;
}

void rasterize(Vector3 v1, Vector3 v2, Vector3 v3, std::vector<double>& zbuffer, TGAImage& framebuffer, TGAColor color){
    double s1, s2, s3;

    auto inside = [&](double x, double y) -> bool {
        s1 = x * (v2[1] - v3[1]) + v2[0] * (v3[1] - y) + v3[0] * (y - v2[1]);
        s2 = x * (v3[1] - v1[1]) + v3[0] * (v1[1] - y) + v1[0] * (y - v3[1]);
        s3 = x * (v1[1] - v2[1]) + v1[0] * (v2[1] - y) + v2[0] * (y - v1[1]);
        return s1 * s2 >= 0 && s1 * s3 >= 0 && s2 * s3 >= 0;
    };

    Matrix3 ABC;
    ABC << v1[0], v2[0], v3[0],
           v1[1], v2[1], v3[1],
           1,    1,    1;
    double signed_area = ABC.det();
    if (signed_area < 1e-5) return;

    int left = static_cast<int>(std::floor(std::min({v1[0], v2[0], v3[0]})));
    int right = static_cast<int>(std::ceil(std::max({v1[0], v2[0], v3[0]})));
    int bottom = static_cast<int>(std::floor(std::min({v1[1], v2[1], v3[1]})));
    int top = static_cast<int>(std::ceil(std::max({v1[1], v2[1], v3[1]})));

    int width = framebuffer.width();
    int height = framebuffer.height();

    for (int i = std::max(0, left); i <= std::min(width - 1, right); i++) {
        for (int j = std::max(0, bottom); j <= std::min(height - 1, top); j++) {
            if (inside(i + 0.5, j + 0.5)) {
                double alpha = s1 / signed_area;
                double beta = s2 / signed_area;
                double gamma = s3 / signed_area;
                double z = alpha * v1[2] + beta * v2[2] + gamma * v3[2];
                if (z < zbuffer[i + j * width]) { // 从 [-f, -n] 映射到 [-1, 1]，-f对应1，-n对应-1，因此z越小表示越近 
                    zbuffer[i + j * width] = z;
                    framebuffer.set(i, j, color);
                }
            }
        }
    }
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <obj_file_path>\n";
        return 1;
    }
    
    srand(time(0));
    constexpr int width  = 1024;
    constexpr int height = 1024;

    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width * height, std::numeric_limits<double>::infinity());

    Vector3 eye{-1, 0, 2}, center{0, 0, 0}, up{0, 1, 0};

    LookAt(eye, center, up);
    viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    perspective(PI / 3, width / static_cast<double>(height), 1, 10);

    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);
        for (int i = 0; i < model.nfaces(); i++) {
            Vector3 window_coords[3];
            for (int j = 0; j < 3; j++) {
                Vector3 v = model.vert(i, j);
                Vector4 v4 = Projection.multiply(ModelView.multiply(Vector4{v[0], v[1], v[2], 1}));
                v4 = v4 * (1.0 / v4[3]);
                Vector4 vp = Viewport.multiply(v4);
                window_coords[j] = {vp[0], vp[1], vp[2]};
            }
            rasterize(window_coords[0], window_coords[1], window_coords[2], zbuffer, framebuffer, get_random_color());
        }
    }

    std::string output_filename = "framebuffer.tga";
    std::filesystem::path name = std::filesystem::path(argv[1]).stem();
    output_filename = name.string() + ".tga";
    framebuffer.write_tga_file(output_filename);

    return 0;
}
