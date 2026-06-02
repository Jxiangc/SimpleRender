# SimpleRender

一个从零手写的软光栅化渲染器，基于 [tinyrenderer](https://github.com/ssloy/tinyrenderer) 教程，使用 C++20 实现。

## 功能

- **OBJ 模型加载** — 支持顶点、法线、纹理坐标的解析，自动加载关联贴图
- **可编程渲染管线** — 着色器抽象（`IShader`），顶点着色器管理 varying 数据，片元着色器逐像素计算
- **Blinn-Phong 光照** — 环境光 / 漫反射 / 高光三分量模型，支持漫反射和高光贴图混合
- **切线空间法线贴图** — TBN 矩阵将切线空间法线变换到视图空间，UV 退化面自动回退到顶点法线
- **Alpha 裁剪** — RGBA 漫反射贴图支持透明片段丢弃（如眼角膜透出虹膜）
- **TGA 图像输出** — 渲染结果写入 TGA 格式，支持灰度 / RGB / RGBA 贴图读取

## 构建

```bash
cmake -B build
cmake --build build --config Release
```

## 运行

```bash
./build/SimpleRender <model.obj> [model2.obj] ...
```

贴图通过 OBJ 文件名自动发现，需放在同一目录下：

| 贴图类型 | 文件命名 |
|---------|---------|
| 切线空间法线贴图 | `<model>_nm_tangent.tga` |
| 漫反射贴图 | `<model>_diffuse.tga` |
| 高光贴图 | `<model>_spec.tga` |

示例：

```bash
./build/SimpleRender obj/african_head/african_head.obj obj/african_head/african_head_eye_outer.obj obj/african_head/african_head_eye_inner.obj
```

## 依赖

- C++20 编译器（GCC 13+ / Clang 16+ / MSVC 2022+）
- CMake 3.10+

## 结构

| 文件 | 说明 |
|------|------|
| `main.cpp` | 程序入口，定义 SimpleShader（TBN 计算、光照、纹理混合） |
| `simplegl.cpp/hpp` | 渲染管线：光栅化、深度测试、透视投影、坐标变换 |
| `model.cpp/hpp` | OBJ 模型解析、顶点数据、贴图自动加载 |
| `tgaimage.cpp/hpp` | TGA 图像读写，支持灰度/RGB/RGBA |
| `linearalgebra.hpp` | 向量 / 矩阵运算模板，支持行列式、求逆、叉积 |

## 提交历史

1. **Initial commit** — 基础软渲染器，OBJ 加载，线框 / 随机色三角面渲染
2. **Refactor** — 渲染管线抽取为 `simplegl` 模块，引入 `IShader` 着色器抽象
3. **Normal map** — 物体空间法线贴图 + Blinn-Phong 逐像素光照
4. **Shader pipeline refactor** — 重构着色器接口，贴图由 Model 管理，简化 `rasterize` 签名，法线贴图法线经 `NormalMatrix` 变换，光照与漫反射/高光贴图混合，Alpha 裁剪
5. **Tangent-space normal mapping** — 切换到切线空间法线贴图，实现 TBN 矩阵变换，UV 退化面回退到顶点法线
