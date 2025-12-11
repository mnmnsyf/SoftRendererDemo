#include "Rasterizer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <tuple> 

// ==========================================
// RGSS 采样点分布 (在 0~1 的像素空间内)
// ==========================================
static const float rgss_offsets[4][2] = {
	{0.125f, 0.625f},
	{0.375f, 0.125f},
	{0.625f, 0.875f},
	{0.875f, 0.375f}
};

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
	// Buffer 大小 = 像素总数 * 每个像素的采样点数
	int total_samples = w * h * SAMPLE_COUNT;
	frame_buffer.resize(total_samples, Vec3f(0, 0, 0));
	depth_buffer.resize(total_samples);
}

void Rasterizer::clear(const Vec3f& color) {
	std::fill(frame_buffer.begin(), frame_buffer.end(), color);
	std::fill(depth_buffer.begin(), depth_buffer.end(), std::numeric_limits<float>::infinity());
}

// 这里的索引计算逻辑变了
// 每个像素占用 4 个连续的位置
int Rasterizer::get_index(int x, int y, int sample_idx) {
	return (y * width + x) * SAMPLE_COUNT + sample_idx;
}

void Rasterizer::set_pixel(int x, int y, const Vec3f& color) {
	if (x < 0 || x >= width || y < 0 || y >= height) return;

	// 如果强制设置像素颜色（比如画点），我们将 4 个采样点都涂满
	for (int i = 0; i < SAMPLE_COUNT; i++) {
		int idx = get_index(x, y, i);
		frame_buffer[idx] = color;
		// 注意：手动 set_pixel 通常不处理深度，或者默认为最前，这里暂且略过深度更新
	}
}

static float cross_product_2d(float x1, float y1, float x2, float y2) {
	return x1 * y2 - x2 * y1;
}

bool Rasterizer::inside(float x, float y, const Vec3f* _v) {
	const Vec3f& A = _v[0];
	const Vec3f& B = _v[1];
	const Vec3f& C = _v[2];

	float cp1 = cross_product_2d(B.x - A.x, B.y - A.y, x - A.x, y - A.y);
	float cp2 = cross_product_2d(C.x - B.x, C.y - B.y, x - B.x, y - B.y);
	float cp3 = cross_product_2d(A.x - C.x, A.y - C.y, x - C.x, y - C.y);

	return (cp1 >= 0 && cp2 >= 0 && cp3 >= 0) || (cp1 <= 0 && cp2 <= 0 && cp3 <= 0);
}

static std::tuple<float, float, float> compute_barycentric_2d(float x, float y, const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
	float c1 = (v1.y - v2.y) * v0.x + (v2.x - v1.x) * v0.y + v1.x * v2.y - v2.x * v1.y;
	if (std::abs(c1) < 1e-5) return { -1, 1, 1 };

	float c2 = (v1.y - v2.y) * x + (v2.x - v1.x) * y + v1.x * v2.y - v2.x * v1.y;
	float alpha = c2 / c1;

	float c3 = (v2.y - v0.y) * x + (v0.x - v2.x) * y + v2.x * v0.y - v0.x * v2.y;
	float beta = c3 / c1;

	return { alpha, beta, 1.0f - alpha - beta };
}

// =============================================
// 核心：画三角形 (RGSS 版)
// =============================================
void Rasterizer::draw_triangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Vec3f* colors) {

	// 1. 包围盒计算 (直接在屏幕坐标系下，不需要放大了)
	float min_x = std::min({ v0.x, v1.x, v2.x });
	float max_x = std::max({ v0.x, v1.x, v2.x });
	float min_y = std::min({ v0.y, v1.y, v2.y });
	float max_y = std::max({ v0.y, v1.y, v2.y });

	int x_start = std::max(0, (int)floor(min_x));
	int x_end = std::min(width - 1, (int)ceil(max_x));
	int y_start = std::max(0, (int)floor(min_y));
	int y_end = std::min(height - 1, (int)ceil(max_y));

	Vec3f v_arr[3] = { v0, v1, v2 };

	// 2. 遍历像素
	for (int y = y_start; y <= y_end; y++) {
		for (int x = x_start; x <= x_end; x++) {

			// 3. 对于每个像素，遍历 4 个 RGSS 采样点
			for (int k = 0; k < SAMPLE_COUNT; k++) {

				// 计算采样点的精确浮点坐标
				// 像素整数坐标 (x,y) + RGSS 偏移量
				float sample_x = x + rgss_offsets[k][0];
				float sample_y = y + rgss_offsets[k][1];

				// 检查该采样点是否在三角形内
				if (inside(sample_x, sample_y, v_arr)) {

					// 计算重心坐标
					auto [alpha, beta, gamma] = compute_barycentric_2d(sample_x, sample_y, v0, v1, v2);

					// Z 值插值
					float z_interpolated = alpha * v0.z + beta * v1.z + gamma * v2.z;

					// 获取该采样点的 Buffer 索引
					int idx = get_index(x, y, k);

					// 4. 独立的深度测试
					if (z_interpolated < depth_buffer[idx]) {
						depth_buffer[idx] = z_interpolated;

						// 颜色插值并写入
						Vec3f interp_color = colors[0] * alpha + colors[1] * beta + colors[2] * gamma;
						frame_buffer[idx] = interp_color;
					}
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

			Vec3f color_sum(0, 0, 0);

			// 汇总 4 个采样点
			for (int k = 0; k < SAMPLE_COUNT; k++) {
				int idx = get_index(x, y, k);
				color_sum = color_sum + frame_buffer[idx];
			}

			// 取平均
			Vec3f final_color = color_sum * 0.25f; // 除以 4

			int r = std::min(255, std::max(0, (int)(final_color.x * 255.0f)));
			int g = std::min(255, std::max(0, (int)(final_color.y * 255.0f)));
			int b = std::min(255, std::max(0, (int)(final_color.z * 255.0f)));
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
			int idx = get_index(x, y, 0);
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