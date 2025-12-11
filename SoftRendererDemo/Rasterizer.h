#pragma once
#include <vector>
#include <string>
#include <limits>
#include "Math.h"

class Rasterizer {
public:
	// 构造函数：传入的是逻辑分辨率（最终输出图片的大小）
	Rasterizer(int w, int h);

	// 清空画布
	void clear(const Vec3f& color);

	// 设置像素颜色（写入到超采样的 FrameBuffer）
	// 注意：这里的 x, y 是超采样后的坐标 (0 ~ width*2)
	void set_pixel(int x, int y, const Vec3f& color);

	// 核心：画三角形
	// 输入：逻辑屏幕坐标（0 ~ width）
	// 内部会自动放大坐标进行 SSAA 渲染
	void draw_triangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Vec3f* colors);

	// 将结果保存为 PPM
	// 此步骤包含 Resolve（降采样）：将 2x2 的像素合并为一个，实现抗锯齿
	void save_to_ppm(const char* filename);

	// 保存深度图
	void save_depth_to_ppm(const char* filename);

private:
	int width, height; // 逻辑宽高

	// SSAA 倍率 (2 表示 2x2 超采样)
	const int SSAA = 2;

	// Frame buffer: 存储 SSAA * width * SSAA * height 的颜色
	std::vector<Vec3f> frame_buffer;

	// Depth buffer: 存储 SSAA * width * SSAA * height 的深度
	std::vector<float> depth_buffer;

	// 辅助：获取 1D 索引 (基于超采样后的宽高)
	int get_index(int x, int y);

	// 辅助：判断点是否在三角形内
	bool inside(float x, float y, const Vec3f* _v);
};