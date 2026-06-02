#include "simplegl.hpp"

Matrix4 ModelView, Viewport, Projection;
Matrix3 NormalMatrix;
std::vector<double> zbuffer;

void lookat(Vector3 eye, Vector3 center, Vector3 up) {
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

    // 法线矩阵是模型视图矩阵的逆转置矩阵，但由于我们没有进行非均匀缩放，所以法线矩阵等于模型视图矩阵的左上3x3部分
    NormalMatrix << x[0], x[1], x[2],
                    y[0], y[1], y[2],
                    z[0], z[1], z[2];
}

void init_viewport(int X, int Y, int w, int h) {
    // X, Y 是视口左下角的坐标，w, h 是视口的宽和高
    Viewport << w / 2.0, 0, 0, X + w / 2.0,
                0, h / 2.0, 0, Y + h / 2.0,
                0, 0, 1, 0,
                0, 0, 0, 1;
}

void init_perspective(double fovY, double aspect, double near, double far) {
    double f = 1.0 / std::tan(fovY / 2);
    Projection << f / aspect, 0, 0, 0,
                  0, f, 0, 0,
                  0, 0, (far + near) / (near - far), (2 * far * near) / (near - far),
                  0, 0, -1, 0;
}

void init_zbuffer(int width, int height) {
    zbuffer.assign(width * height, std::numeric_limits<double>::infinity());
}

void line(int ax, int ay, int bx, int by, TGAImage& framebuffer, TGAColor color) {
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

void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer) {
    Vector4 ndc[3] = { clip[0] / clip[0][3], clip[1] / clip[1][3], clip[2] / clip[2][3] };
    Vector2 screen[3] = { 
        Viewport.multiply(ndc[0]).xy(),
        Viewport.multiply(ndc[1]).xy(),
        Viewport.multiply(ndc[2]).xy()
    };

    auto area = [&](Vector2 a, Vector2 b, Vector2 c) -> double {
        return (a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1]));
    };

    double s1, s2, s3;
    auto inside = [&](double x, double y) -> bool {
        s1 = area({x, y}, screen[1], screen[2]);
        s2 = area(screen[0], {x, y}, screen[2]);
        s3 = area(screen[0], screen[1], {x, y});
        return s1 * s2 >= 0 && s1 * s3 >= 0 && s2 * s3 >= 0;
    };

    double signed_area = area(screen[0], screen[1], screen[2]);
    if (signed_area < 1) return;

    int left = static_cast<int>(std::floor(std::min({screen[0][0], screen[1][0], screen[2][0]})));
    int right = static_cast<int>(std::ceil(std::max({screen[0][0], screen[1][0], screen[2][0]})));
    int bottom = static_cast<int>(std::floor(std::min({screen[0][1], screen[1][1], screen[2][1]})));
    int top = static_cast<int>(std::ceil(std::max({screen[0][1], screen[1][1], screen[2][1]})));

    int width = framebuffer.width();
    int height = framebuffer.height();

#pragma omp parallel for
    for (int i = std::max(0, left); i <= std::min(width - 1, right); i++) {
        for (int j = std::max(0, bottom); j <= std::min(height - 1, top); j++) {
            if (inside(i + 0.5, j + 0.5)) {
                double alpha = s1 / signed_area;
                double beta = s2 / signed_area;
                double gamma = s3 / signed_area;
                double z = alpha * ndc[0][2] + beta * ndc[1][2] + gamma * ndc[2][2];
                if (z >= zbuffer[i + j * width]) continue; // 从 [-f, -n] 映射到 [-1, 1]，-f对应1，-n对应-1，因此z越小表示越近 
                auto [bc, color] = shader.fragment({alpha, beta, gamma});
                if (bc) continue; // 如果片段着色器返回true，表示该片段被丢弃
                framebuffer.set(i, j, color);
                zbuffer[i + j * width] = z;
            }
        }
    }
}