#include "simplegl.hpp"

Matrix4 ModelView, Viewport, Projection;
Matrix3 NormalMatrix;
Matrix4 light_MVP;
std::vector<double> zbuffer;
std::vector<double> lightzbuffer;

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

void init_lightzbuffer(int w, int h) {
    lightzbuffer.assign(w * h, std::numeric_limits<double>::infinity());
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

struct TriangleSetup {
    Vector4 ndc[3];
    Vector2 screen[3];
    double signed_area;
    int left, right, bottom, top;

    TriangleSetup(const Triangle& clip) {
        ndc[0] = clip[0] / clip[0][3]; ndc[1] = clip[1] / clip[1][3]; ndc[2] = clip[2] / clip[2][3];
        screen[0] = Viewport.multiply(ndc[0]).xy();
        screen[1] = Viewport.multiply(ndc[1]).xy();
        screen[2] = Viewport.multiply(ndc[2]).xy();

        signed_area = area2(screen[0], screen[1], screen[2]);
        left   = static_cast<int>(std::floor(std::min({screen[0][0], screen[1][0], screen[2][0]})));
        right  = static_cast<int>(std::ceil (std::max({screen[0][0], screen[1][0], screen[2][0]})));
        bottom = static_cast<int>(std::floor(std::min({screen[0][1], screen[1][1], screen[2][1]})));
        top    = static_cast<int>(std::ceil (std::max({screen[0][1], screen[1][1], screen[2][1]})));
    }

    bool valid() const { return signed_area >= 1; }

    static double area2(Vector2 a, Vector2 b, Vector2 c) {
        return a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1]);
    }

    bool inside(double& s1, double& s2, double& s3, double x, double y) const {
        s1 = area2({x, y}, screen[1], screen[2]);
        s2 = area2(screen[0], {x, y}, screen[2]);
        s3 = area2(screen[0], screen[1], {x, y});
        return s1 * s2 >= 0 && s1 * s3 >= 0 && s2 * s3 >= 0;
    }
};

void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer) {
    TriangleSetup setup(clip);
    if (!setup.valid()) return;

    int w = framebuffer.width(), h = framebuffer.height();
    double S = setup.signed_area;

#pragma omp parallel for
    for (int i = std::max(0, setup.left); i <= std::min(w - 1, setup.right); i++) {
        for (int j = std::max(0, setup.bottom); j <= std::min(h - 1, setup.top); j++) {
            double s1, s2, s3;
            if (!setup.inside(s1, s2, s3, i + 0.5, j + 0.5)) continue;
            double alpha = s1 / S, beta = s2 / S, gamma = s3 / S;
            double z = alpha * setup.ndc[0][2] + beta * setup.ndc[1][2] + gamma * setup.ndc[2][2];
            if (z >= zbuffer[i + j * w]) continue;
            auto [bc, color] = shader.fragment({alpha, beta, gamma});
            if (bc) continue;
            framebuffer.set(i, j, color);
            zbuffer[i + j * w] = z;
        }
    }
}

void rasterize_depth(const Triangle& clip, int w, int h) {
    TriangleSetup setup(clip);
    if (!setup.valid()) return;

    double S = setup.signed_area;

#pragma omp parallel for
    for (int i = std::max(0, setup.left); i <= std::min(w - 1, setup.right); i++) {
        for (int j = std::max(0, setup.bottom); j <= std::min(h - 1, setup.top); j++) {
            double s1, s2, s3;
            if (!setup.inside(s1, s2, s3, i + 0.5, j + 0.5)) continue;
            double z = (s1 * setup.ndc[0][2] + s2 * setup.ndc[1][2] + s3 * setup.ndc[2][2]) / S;
            if (z >= lightzbuffer[i + j * w]) continue;
            lightzbuffer[i + j * w] = z;
        }
    }
}