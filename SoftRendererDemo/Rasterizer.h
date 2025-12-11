#pragma once
#include <vector>
#include <string>
#include <limits>
#include "Math.h"

class Rasterizer {
public:
    // 构造函数：初始化屏幕宽高
    Rasterizer(int w, int h);

    // 清空画布 (通常用黑色或一种背景色填充)
    void clear(const Vec3f& color);

    // 设置像素颜色 (写入 FrameBuffer)
    // 做了边界检查，防止画到屏幕外面导致崩溃
    void set_pixel(int x, int y, const Vec3f& color);

    // 核心：画三角形
    // 输入：三个屏幕空间的顶点坐标 + 颜色
    // colors 是一个包含 3 个 Vec3f 的数组，分别对应 v0, v1, v2 的颜色
    void draw_triangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Vec3f* colors);

    // 将 FrameBuffer 导出为 .ppm 图片文件
    // PPM 是一种极简单的图片格式，不需要任何第三方库就能写
    void save_to_ppm(const char* filename);

    // 将深度缓冲区导出为灰度图
    void save_depth_to_ppm(const char* filename);

private:
    int width, height;

    // 帧缓冲区：本质上就是一个长长的一维数组，存了 w * h 个颜色
    std::vector<Vec3f> frame_buffer;

	// 深度缓冲区：存储每个像素当前的最小深度值 (Z值)
	std::vector<float> depth_buffer;

    // 辅助函数：计算一维索引
    int get_index(int x, int y);

    // 辅助函数：判断点 (x, y) 是否在三角形内部
    // _v 是包含三个顶点的数组指针
    bool inside(float x, float y, const Vec3f* _v);
};