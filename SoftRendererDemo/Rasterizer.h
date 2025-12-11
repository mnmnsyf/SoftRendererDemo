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

	// 设置像素颜色
	// 为了兼容性，如果直接设置一个像素，我们会把该像素内的 4 个采样点都设为这个颜色
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

	// 这里的 4 代表 RGSS 的 4 个采样点
	const int SAMPLE_COUNT = 4;

	// Flattened Buffer:
	// 内存布局：Pixel_0[Sample0, Sample1, Sample2, Sample3], Pixel_1[...]
	// 总大小 = width * height * 4
	std::vector<Vec3f> frame_buffer;
	std::vector<float> depth_buffer;

	// 获取特定像素的特定采样点的索引
	// sample_idx 范围 0~3
	int get_index(int x, int y, int sample_idx);

	// 辅助：判断点是否在三角形内
	bool inside(float x, float y, const Vec3f* _v);
};