#pragma once
#include <algorithm>

#include "linearalgebra.hpp"
#include "tgaimage.hpp"

void lookat(Vector3 eye, Vector3 center, Vector3 up);
void init_perspective(double fov, double aspect, double near, double far);
void init_viewport(int X, int Y, int w, int h);
void init_zbuffer(int width, int height);

class IShader {
public:
    virtual std::pair<bool, TGAColor> fragment(const Vector3& bar) const = 0;
};

typedef Vector4 Triangle[3];
void line(int ax, int ay, int bx, int by, TGAImage& image, TGAColor color);
void rasterize(const Triangle& clip, const IShader& shader, TGAImage& image);