#include "Rasterizer.h"
#include <algorithm> // for std::min, std::max
#include <fstream>   // for file writing
#include <iostream>
#include <tuple> 

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
	// 【关键修改】Buffer 大小初始化为 (w*2) * (h*2)
	// 这样每个逻辑像素对应 4 个物理子像素，拥有独立的 Z-Buffer
	int physical_w = w * SSAA;
	int physical_h = h * SSAA;

	frame_buffer.resize(physical_w * physical_h, Vec3f(0, 0, 0));
	depth_buffer.resize(physical_w * physical_h);
}

void Rasterizer::clear(const Vec3f& color) {
	std::fill(frame_buffer.begin(), frame_buffer.end(), color);
	std::fill(depth_buffer.begin(), depth_buffer.end(), std::numeric_limits<float>::infinity());
}

// 获取索引时，使用超采样后的宽度
int Rasterizer::get_index(int x, int y) {
	return y * (width * SSAA) + x;
}

void Rasterizer::set_pixel(int x, int y, const Vec3f& color) {
	// 边界检查：注意这里检查的是超采样后的边界
	if (x < 0 || x >= width * SSAA || y < 0 || y >= height * SSAA) return;
	int index = get_index(x, y);
	frame_buffer[index] = color;
}

// 叉乘计算 (未修改)
static float cross_product_2d(float x1, float y1, float x2, float y2) {
	return x1 * y2 - x2 * y1;
}

// 判断点是否在三角形内 (未修改，数学逻辑通用)
bool Rasterizer::inside(float x, float y, const Vec3f* _v) {
	const Vec3f& A = _v[0];
	const Vec3f& B = _v[1];
	const Vec3f& C = _v[2];

	float cp1 = cross_product_2d(B.x - A.x, B.y - A.y, x - A.x, y - A.y);
	float cp2 = cross_product_2d(C.x - B.x, C.y - B.y, x - B.x, y - B.y);
	float cp3 = cross_product_2d(A.x - C.x, A.y - C.y, x - C.x, y - C.y);

	return (cp1 >= 0 && cp2 >= 0 && cp3 >= 0) || (cp1 <= 0 && cp2 <= 0 && cp3 <= 0);
}

// 重心坐标计算 (未修改，数学逻辑通用)
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
// 核心：画三角形 (SSAA 版)
// =============================================
void Rasterizer::draw_triangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Vec3f* colors) {

	// 1. 【关键】坐标放大
	// 将逻辑坐标 (0~800) 映射到 超采样坐标 (0~1600)
	// Z 值不需要放大，因为它是深度
	Vec3f s_v0 = v0; s_v0.x *= SSAA; s_v0.y *= SSAA;
	Vec3f s_v1 = v1; s_v1.x *= SSAA; s_v1.y *= SSAA;
	Vec3f s_v2 = v2; s_v2.x *= SSAA; s_v2.y *= SSAA;

	// 2. 计算包围盒 (Bounding Box) - 在超采样空间计算
	float min_x = std::min({ s_v0.x, s_v1.x, s_v2.x });
	float max_x = std::max({ s_v0.x, s_v1.x, s_v2.x });
	float min_y = std::min({ s_v0.y, s_v1.y, s_v2.y });
	float max_y = std::max({ s_v0.y, s_v1.y, s_v2.y });

	// 限制在超采样 Buffer 范围内
	int x_start = std::max(0, (int)floor(min_x));
	int x_end = std::min(width * SSAA - 1, (int)ceil(max_x));
	int y_start = std::max(0, (int)floor(min_y));
	int y_end = std::min(height * SSAA - 1, (int)ceil(max_y));

	Vec3f v_arr[3] = { s_v0, s_v1, s_v2 };

	// 3. 遍历每一个“子像素” (Sub-pixel)
	// 此时对于代码逻辑来说，这就是在画一个巨大的图，不需要考虑混合，只需要考虑 Z-Test
	for (int y = y_start; y <= y_end; y++) {
		for (int x = x_start; x <= x_end; x++) {

			// 像素中心
			float px = x + 0.5f;
			float py = y + 0.5f;

			// 检查是否在三角形内
			if (inside(px, py, v_arr)) {

				// 计算重心坐标 (用于插值 Z 和 颜色)
				auto [alpha, beta, gamma] = compute_barycentric_2d(px, py, s_v0, s_v1, s_v2);

				// 插值 Z
				float z_interpolated = alpha * s_v0.z + beta * s_v1.z + gamma * s_v2.z;

				// 获取当前子像素的 Buffer 索引
				int index = get_index(x, y);

				// 4. 标准 Z-Buffer 测试
				// 每个子像素都有自己独立的 Z 值，互不干扰！
				if (z_interpolated < depth_buffer[index]) {

					// 通过测试：更新 Z
					depth_buffer[index] = z_interpolated;

					// 插值颜色
					Vec3f interp_color = colors[0] * alpha + colors[1] * beta + colors[2] * gamma;

					// 写入颜色 (不进行混合，直接覆盖)
					set_pixel(x, y, interp_color);
				}
			}
		}
	}
}

// =============================================
// 保存 PPM (包含 Downsampling / Resolve 过程)
// =============================================
void Rasterizer::save_to_ppm(const char* filename) {
	std::ofstream ofs(filename);
	ofs << "P3\n" << width << " " << height << "\n255\n";

	// 遍历逻辑像素 (最终输出大小)
	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {

			Vec3f color_sum(0, 0, 0);

			// 【关键】SSAA 降采样 (Resolve)
			// 汇总当前逻辑像素对应的 4 个 (SSAAxSSAA) 物理子像素的颜色
			for (int sy = 0; sy < SSAA; sy++) {
				for (int sx = 0; sx < SSAA; sx++) {
					// 计算子像素的坐标
					int sub_x = x * SSAA + sx;
					int sub_y = y * SSAA + sy;

					int index = get_index(sub_x, sub_y);
					color_sum = color_sum + frame_buffer[index];
				}
			}

			// 求平均值
			Vec3f final_color = color_sum * (1.0f / (SSAA * SSAA));

			// 写入文件
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

	// 寻找最大最小Z (用于归一化可视化)
	for (float z : depth_buffer) {
		if (z != std::numeric_limits<float>::infinity()) {
			if (z < min_z) min_z = z;
			if (z > max_z) max_z = z;
		}
	}
	if (min_z == std::numeric_limits<float>::infinity()) { min_z = 0; max_z = 1; }
	if (min_z == max_z) max_z = min_z + 0.0001f;
	float range = max_z - min_z;

	// 遍历逻辑像素
	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {

			// 对于深度图，通常取中间那个采样点的深度，或者平均深度
			// 这里为了简单，我们取 (0,0) 位置的子像素深度代表整个像素
			int sub_x = x * SSAA;
			int sub_y = y * SSAA;
			int index = get_index(sub_x, sub_y);

			float z = depth_buffer[index];
			int gray = 0;

			if (z != std::numeric_limits<float>::infinity()) {
				float factor = (z - min_z) / range;
				factor = std::max(0.0f, std::min(1.0f, factor));
				gray = static_cast<int>(factor * 255);
			}
			else {
				gray = 255; // 无限远显示为白
			}

			ofs << gray << " " << gray << " " << gray << " ";
		}
		ofs << "\n";
	}
	ofs.close();
}