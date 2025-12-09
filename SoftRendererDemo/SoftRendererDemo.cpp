// SoftRendererDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "Math.h"

int main()
{
    // 1. 设置场景
     // 我们的物体在 (0, 0, -5) 的位置 (屏幕里面5米)
    Vec4f worldPos(0.0f, 0.0f, -5.0f, 1.0f);

    // 2. 构建 View Matrix (摄像机)
    // 摄像机在原点 (0,0,0)，看向 (0,0,-1)，头顶朝上 (0,1,0)
    Vec3f eye(0, 0, 0);
    Vec3f center(0, 0, -1);
    Vec3f up(0, 1, 0);
    Mat4 view = Mat4::lookAt(eye, center, up);

    // 3. 构建 Projection Matrix (透视)
    // 45度视角，16:9 屏幕，最近能看 0.1米，最远能看 100米
    Mat4 proj = Mat4::perspective(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);

    // 4. 组合矩阵 (MVP)
    // 假设模型矩阵是单位矩阵 (物体没动)
    Mat4 model = Mat4::identity();
    Mat4 mvp = proj * view * model; // 注意顺序！P * V * M

    // 5. 变换点
    Vec4f clipPos = mvp * worldPos;

    std::cout << "Clip Space Pos: "
        << clipPos.x << ", " << clipPos.y << ", "
        << clipPos.z << ", " << clipPos.w << std::endl;

    // 6. 模拟 GPU 的“透视除法” (Perspective Divide)
    // 这一步通常是显卡自动做的，把坐标变回 -1 到 1 的范围内
    if (clipPos.w != 0) {
        float x_ndc = clipPos.x / clipPos.w;
        float y_ndc = clipPos.y / clipPos.w;
        std::cout << "Screen(NDC) Pos: " << x_ndc << ", " << y_ndc << std::endl;
        // 预期：因为物体在正中央，xy 应该是 (0, 0)
    }

    return 0;
}