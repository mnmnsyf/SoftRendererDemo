#include <iostream>
#include "Math.h"
#include "Rasterizer.h" // 引入光栅化器

int main() {
    // 1. 初始化光栅化器 (800x600 分辨率)
    const int width = 800;
    const int height = 600;
    Rasterizer r(width, height);

    // 2. 准备 MVP 矩阵 (和之前完全一样)
    Vec3f eye(0, 0, 0);
    Vec3f center(0, 0, -1);
    Vec3f up(0, 1, 0);
    Mat4 view = Mat4::lookAt(eye, center, up);
    Mat4 proj = Mat4::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
    Mat4 viewport = Mat4::viewport(width, height);

    // 3. 定义一个三角形 (位于 Z = -5)
    Vec4f v1(0.0f, 0.5f, -2.0f, 1.0f); // 顶
    Vec4f v2(-0.5f, -0.5f, -2.0f, 1.0f); // 左下
    Vec4f v3(0.5f, -0.5f, -2.0f, 1.0f); // 右下
    Vec4f raw_verts[] = { v1, v2, v3 };

    // 【新增】定义三个顶点的颜色 (RGB)
    Vec3f c1(1.0f, 0.0f, 0.0f); // 红色
    Vec3f c2(0.0f, 1.0f, 0.0f); // 绿色
    Vec3f c3(0.0f, 0.0f, 1.0f); // 蓝色
    Vec3f colors[] = { c1, c2, c3 };

    // 用于存储变换后的屏幕坐标
    Vec3f screen_verts[3];

    // 4. 顶点着色器流程 (Vertex Shader Pipeline)
    for (int i = 0; i < 3; i++) {
        // MVP 变换 -> Clip Space
        Vec4f clip = proj * view * raw_verts[i];

        // 透视除法 -> NDC
        // 注意：这里除以 w 非常关键，否则没有近大远小
        Vec3f ndc;
        ndc.x = clip.x / clip.w;
        ndc.y = clip.y / clip.w;
        ndc.z = clip.z / clip.w;

        // 视口变换 -> Screen Space
        Vec4f screen = viewport * Vec4f(ndc.x, ndc.y, ndc.z, 1.0f);

        // 存入数组 (只取 x, y, z)
        screen_verts[i] = Vec3f(screen.x, screen.y, screen.z);
    }

    // 5. 光栅化 (Rasterization)
    // 传入三个屏幕坐标，和一个颜色 (比如绿色)
    r.clear(Vec3f(0.1f, 0.1f, 0.1f)); // 背景设为深灰色
    r.draw_triangle(screen_verts[0], screen_verts[1], screen_verts[2], colors);

    // 6. 保存结果
    r.save_to_ppm("rainbow_triangle.ppm");

    return 0;
}