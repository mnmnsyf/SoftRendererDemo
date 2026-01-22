#pragma once
#include <vector>
#include <string>
#include <limits>
#include "GMath.h"

// ==========================================
// 定义灯光结构
// ==========================================
struct Light {
	Vec3f position;
	Vec3f intensity; // 颜色 * 亮度
};

// ==========================================
// 定义着色器接口
// ==========================================
struct IShader {
	virtual ~IShader() = default;

	// 顶点着色器：
	// 输入：顶点索引 (vert_idx)
	// 输出：裁剪空间坐标 (Vec4f)
	// 副作用：在这个函数里，Shader 会把法线、UV 等数据存到 varying 变量(如法线、世界坐标等)里准备插值
	virtual Vec4f vertex(int iface, size_t vert_idx) = 0;

	// 片元着色器：
	// 输入：重心坐标插值系数 (alpha, beta, gamma)
	// 输出：最终像素颜色 (Vec3f)
	virtual Vec3f fragment(float alpha, float beta, float gamma) = 0;
};

// ==========================================
// Rasterizer 类定义
// ==========================================
class Rasterizer {
public:
	// 构造函数：传入的是逻辑分辨率（最终输出图片的大小）
	Rasterizer(int w, int h);

	// 基础功能
	Vec2f GetScreenSize() const { return Vec2f(width, height); }
	void clear(const Vec3f& color);
	void set_pixel(int x, int y, const Vec3f& color);
	void save_to_ppm(const char* filename);
	void save_depth_to_ppm(const char* filename);

	// 使用 Shader 进行绘制
	// n_verts: 顶点总数 (通常是 3 的倍数)
	void draw(IShader& shader, size_t n_verts);

	// 绘制线框模式
	void draw_wireframe(IShader& shader, size_t n_verts);

private:
	int width, height;
	const int SAMPLE_COUNT = 4; // 依然保留 RGSS 结构，但在 draw_new 中我们暂时简化为单采样

	std::vector<Vec3f> frame_buffer;
	std::vector<float> depth_buffer;

	int get_index(int x, int y);

	// 光栅化一个三角形
	// v: 屏幕空间坐标 (x, y, z_ndc, w_original)
	// w_recip:  三个顶点的 1/w 值，用于透视矫正
	void rasterize_triangle(const Vec4f v[], const float w_recip[], IShader& shader);

	// Bresenham 画线算法 (带有深度测试)
	void draw_line_3d(const Vec3f& p0, const Vec3f& p1, const Vec3f& color);

	// 辅助计算重心坐标
	// 返回 tuple: {alpha, beta, gamma}
	std::tuple<float, float, float> compute_barycentric_2d(float x, float y, const Vec4f* v);

	// 判断是否是背面
	// 输入是经过视口变换后的屏幕空间坐标
	bool is_back_face(const Vec4f& v0, const Vec4f& v1, const Vec4f& v2);
};