#include "Rasterizer.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <tuple>

// ==========================================
// RGSS 采样点分布 (在 0~1 的像素空间内)
// ==========================================
// 特点：X 和 Y 投影都不会重叠，最大限度利用采样数
static const float rgss_offsets[4][2] = {
	{0.125f, 0.625f}, // Sample 0
	{0.375f, 0.125f}, // Sample 1
	{0.625f, 0.875f}, // Sample 2
	{0.875f, 0.375f}  // Sample 3
};

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
	int total_samples = w * h * SAMPLE_COUNT; // 还是分配这么大，兼容旧代码
	frame_buffer.resize(total_samples, Vec3f(0, 0, 0));
	depth_buffer.resize(total_samples);
}

int Rasterizer::get_index(int x, int y) {
	// 为了简化 new draw，我们默认写入第一个采样点的位置，或者不使用 MSAA
	// 这里为了兼容 set_pixel 的逻辑 (y * width + x) * 4
	return (height - 1 - y) * width * 4 + x * 4;
}

void Rasterizer::clear(const Vec3f& color) {
	std::fill(frame_buffer.begin(), frame_buffer.end(), color);
	std::fill(depth_buffer.begin(), depth_buffer.end(), std::numeric_limits<float>::infinity());
}

void Rasterizer::set_pixel(int x, int y, const Vec3f& color) {
	if (x < 0 || x >= width || y < 0 || y >= height) return;
	int idx = get_index(x, y);
	// 简单起见，把该像素的所有采样点都设为同一个颜色
	for (int i = 0; i < SAMPLE_COUNT; i++) {
		frame_buffer[idx + i] = color;
	}
}

// ==========================================
// 新增：Barycentric 坐标计算辅助函数
// ==========================================
std::tuple<float, float, float> Rasterizer::compute_barycentric_2d(float x, float y, const Vec4f* v) {
	float c1 = (x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * y + v[1].x * v[2].y - v[2].x * v[1].y) / (v[0].x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * v[0].y + v[1].x * v[2].y - v[2].x * v[1].y);
	float c2 = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);
	float c3 = 1.0f - c1 - c2;
	return { c1, c2, c3 };
}

bool Rasterizer::is_back_face(const Vec4f& v0, const Vec4f& v1, const Vec4f& v2)
{
	// 1. 计算两边向量 (只关心 X, Y)
	// 向量 A: v0 -> v1
	float ax = v1.x - v0.x;
	float ay = v1.y - v0.y;

	// 向量 B: v0 -> v2
	float bx = v2.x - v0.x;
	float by = v2.y - v0.y;

	// 2. 计算 2D 叉积 (Z分量)
	// Cross Z = Ax * By - Ay * Bx
	float cross_z = ax * by - ay * bx;

	// 3. 判定符号
	// ---------------------------------------------------------
	// 关键逻辑：
	// 标准 OBJ/OpenGL 网格是逆时针 (CCW) 为正面。
	// 但是！我们的屏幕坐标系通常 Y 轴是向下的 (0 在顶部, height 在底部)。
	// 这种坐标系的 Y 轴翻转会导致原本的 "逆时针" 在屏幕上看起来变成 "顺时针"。
	// 
	// 在 Y 向下的坐标系中：
	// - 顺时针 (Visual CW)  -> cross_z > 0
	// - 逆时针 (Visual CCW) -> cross_z < 0
	//
	// 因此，原本是 CCW 的正面三角形，投影后会变成 Visual CW (cross_z > 0)。
	// 所以我们应该保留 > 0 的，剔除 <= 0 的。
	// ---------------------------------------------------------

	// 如果结果 <= 0，说明是背面（或退化三角形），返回 true 进行剔除
	return cross_z <= 0;
}

// ==========================================
// draw 函数：几何处理阶段
// ==========================================
void Rasterizer::draw(IShader& shader, size_t n_verts) {
	// 每次处理 3 个顶点 (GL_TRIANGLES)
	for (size_t i = 0; i < n_verts; i += 3) {

		// A. 顶点着色器 (Vertex Shader) -> 裁剪空间
		Vec4f v_clip[3];
		for (int k = 0; k < 3; ++k) {
			v_clip[k] = shader.vertex(k, i + k);
		}

		// B. 简单的视锥剔除 (Clipping)
		// 只要有一个点在相机背后 (w <= 0)，就丢弃整个三角形
		// (完善的引擎会在这里进行 Viewport Clipping，将三角形切割)
		if (v_clip[0].w <= 0 || v_clip[1].w <= 0 || v_clip[2].w <= 0) {
			continue;
		}

		// C. 准备透视矫正数据
		// 保存 1/w，后续光栅化时插值用
		float w_recip[3];
		for (int k = 0; k < 3; ++k) {
			w_recip[k] = 1.0f / v_clip[k].w;
		}

		// D. 透视除法 (Perspective Division) -> NDC (-1 ~ 1)
		Vec4f v_ndc[3];
		for (int k = 0; k < 3; ++k) {
			v_ndc[k].x = v_clip[k].x * w_recip[k];
			v_ndc[k].y = v_clip[k].y * w_recip[k];
			v_ndc[k].z = v_clip[k].z * w_recip[k];
			v_ndc[k].w = 1.0f; // w 归一化，不再携带深度信息
		}

		// E. 视口变换 (Viewport Transform) -> 屏幕空间
		Vec4f v_screen[3];
		for (int k = 0; k < 3; ++k) {
			// x: [-1, 1] -> [0, width]
			v_screen[k].x = 0.5f * width * (v_ndc[k].x + 1.0f);
			// y: [-1, 1] -> [0, height]
			v_screen[k].y = 0.5f * height * (v_ndc[k].y + 1.0f);
			// z: 保持 NDC z，用于深度测试
			v_screen[k].z = v_ndc[k].z;
			// w: 这里的 w 实际上没用了，但为了数据结构统一先放着
			v_screen[k].w = v_clip[k].w;
		}

		// 背面剔除检查
		if (is_back_face(v_screen[0], v_screen[1], v_screen[2])) {
			continue; // 跳过这个三角形，不画了
		}

		// F. 进入光栅化阶段
		rasterize_triangle(v_screen, w_recip, shader);
	}
}

void Rasterizer::rasterize_triangle(const Vec4f v[], const float w_recip[], IShader& shader) {
	// 1. 包围盒计算 (Bounding Box)
	float min_x = std::min({ v[0].x, v[1].x, v[2].x });
	float max_x = std::max({ v[0].x, v[1].x, v[2].x });
	float min_y = std::min({ v[0].y, v[1].y, v[2].y });
	float max_y = std::max({ v[0].y, v[1].y, v[2].y });

	int x0 = std::max(0, (int)std::floor(min_x));
	int x1 = std::min(width - 1, (int)std::ceil(max_x));
	int y0 = std::max(0, (int)std::floor(min_y));
	int y1 = std::min(height - 1, (int)std::ceil(max_y));

	// 2. 遍历像素
	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {

			// 获取当前像素在 Buffer 中的起始索引 (对应 4 个采样点的第 1 个)
			// 假设 get_index 返回的是像素块的起始位置
			int pixel_base_index = get_index(x, y);

			// === RGSS 采样循环 ===
			for (int k = 0; k < 4; ++k) {
				// A. 计算采样点坐标
				float px = x + rgss_offsets[k][0];
				float py = y + rgss_offsets[k][1];

				// B. 计算屏幕空间重心坐标
				auto [alpha, beta, gamma] = compute_barycentric_2d(px, py, v);

				// C. 覆盖测试 (Inside Test)
				if (alpha < 0 || beta < 0 || gamma < 0) continue;

				// D. 透视矫正核心 (针对当前采样点)
				// ---------------------------------------------------------
				// 这里的 alpha/beta/gamma 是针对该采样点的，所以 correction 也是独立的

				// 插值 1/w
				float interpolated_w_recip = alpha * w_recip[0] + beta * w_recip[1] + gamma * w_recip[2];

				// 为了防止除以 0 (虽然理论上屏幕内的点 w 都在 view frustum 内)
				if (std::abs(interpolated_w_recip) < 1e-5) continue;

				// 计算透视矫正后的权重
				float alpha_p = (alpha * w_recip[0]) / interpolated_w_recip;
				float beta_p = (beta * w_recip[1]) / interpolated_w_recip;
				float gamma_p = (gamma * w_recip[2]) / interpolated_w_recip;

				// E. 深度测试 (Z-Buffer)
				// ---------------------------------------------------------
				// 同样，Z 值也要针对采样点插值
				float z_interpolated = alpha * v[0].z + beta * v[1].z + gamma * v[2].z;

				// 对应的采样点索引
				int sample_index = pixel_base_index + k;

				if (z_interpolated < depth_buffer[sample_index]) {
					// 更新深度
					depth_buffer[sample_index] = z_interpolated;

					// F. 执行 Fragment Shader (SSAA 模式)
					// -----------------------------------------------------
					// 我们为每个采样点都跑一次 Shader。这对于高频纹理（如棋盘格）
					// 来说效果最好，因为能同时解决边缘锯齿和纹理内部锯齿。
					Vec3f color = shader.fragment(alpha_p, beta_p, gamma_p);

					// 写入颜色缓冲
					frame_buffer[sample_index] = color;
				}
			}
		}
	}
}

// =============================================
// 保存 PPM (Downsample / Resolve)
// =============================================
void Rasterizer::save_to_ppm(const char* filename) {
	std::ofstream ofs(filename);
	ofs << "P3\n" << width << " " << height << "\n255\n";

	for (int y = height - 1; y >= 0; y--) { 
		for (int x = 0; x < width; x++) {

			// Resolve: 将 4 个采样点取平均
			Vec3f color(0, 0, 0);
			int idx = get_index(x, y); // 获取块首地址

			for (int k = 0; k < 4; k++) {
				color = color + frame_buffer[idx + k];
			}
			color = color * 0.25f; // 除以 4

			// 转换到 0-255 并写入
			int r = std::min(255, std::max(0, (int)(color.x * 255.0f)));
			int g = std::min(255, std::max(0, (int)(color.y * 255.0f)));
			int b = std::min(255, std::max(0, (int)(color.z * 255.0f)));
			ofs << r << " " << g << " " << b << " ";
		}
		ofs << "\n";
	}
	ofs.close();
	std::cout << "Image saved to " << filename << std::endl;
}

void Rasterizer::save_depth_to_ppm(const char* filename) {
	std::ofstream ofs(filename);
	ofs << "P3\n" << width << " " << height << "\n255\n";

	float min_z = std::numeric_limits<float>::infinity();
	float max_z = -std::numeric_limits<float>::infinity();

	// 计算 Min/Max Z 用于可视化
	for (float z : depth_buffer) {
		if (z != std::numeric_limits<float>::infinity()) {
			if (z < min_z) min_z = z;
			if (z > max_z) max_z = z;
		}
	}
	if (min_z == std::numeric_limits<float>::infinity()) { min_z = 0; max_z = 1; }
	if (min_z == max_z) max_z = min_z + 0.0001f;
	float range = max_z - min_z;

	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {

			// 对于深度图可视化，为了简单起见，我们只取第 1 个采样点 (索引 0)
			// 或者你可以取平均，但这在物理上意义不大
			int idx = get_index(x, y);
			float z = depth_buffer[idx];

			int gray = 0;
			if (z != std::numeric_limits<float>::infinity()) {
				float factor = (z - min_z) / range;
				factor = std::max(0.0f, std::min(1.0f, factor));
				gray = static_cast<int>(factor * 255);
			}
			else {
				gray = 255;
			}
			ofs << gray << " " << gray << " " << gray << " ";
		}
		ofs << "\n";
	}
	ofs.close();
}