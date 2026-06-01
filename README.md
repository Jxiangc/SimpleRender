# SimpleRender

一个从零手写的软光栅化渲染器，基于 [tinyrenderer](https://github.com/ssloy/tinyrenderer) 教程，使用 C++20 实现。

## 功能

- **OBJ 模型加载** — 支持顶点、法线、纹理坐标的解析
- **渲染管线** — 可编程着色器抽象（`IShader`），支持顶点变换和逐像素着色
- **Blinn-Phong 光照** — 环境光 + 漫反射 + 高光的三分量光照模型
- **法线贴图** — 逐像素法线采样，实现低模表面的高细节光照
- **TGA 图像输出** — 渲染结果写入 TGA 格式文件

## 构建

```bash
cmake -B build
cmake --build build --config Release
```

## 运行

```bash
./build/SimpleRender <model.obj> [<normal_map.tga>]
```

示例：

```bash
./build/SimpleRender obj/african_head/african_head.obj obj/african_head/african_head_nm.tga
```

## 依赖

- C++20 编译器（GCC 13+ / Clang 16+ / MSVC 2022+）
- CMake 3.10+

## 结构

| 文件 | 说明 |
|------|------|
| `main.cpp` | 程序入口，定义着色器与渲染循环 |
| `simplegl.cpp/hpp` | 渲染管线：光栅化、着色器接口、数学变换 |
| `model.cpp/hpp` | OBJ 模型解析与顶点数据管理 |
| `tgaimage.cpp/hpp` | TGA 图像读写 |
| `linearalgebra.hpp` | 向量 / 矩阵运算（Vector3、Matrix4 等） |

## 提交历史

1. **Initial commit** — 基础软渲染器，支持 OBJ 加载和线框/三角面渲染
2. **Refactor** — 将渲染管线抽取为 `simplegl` 模块，引入着色器抽象
3. **Normal map** — 添加法线贴图支持，实现 Blinn-Phong 逐像素光照
4. **C++20** — 配置 CMake 使用 C++20 标准，修复 `std::format` / `ends_with` 构建错误
