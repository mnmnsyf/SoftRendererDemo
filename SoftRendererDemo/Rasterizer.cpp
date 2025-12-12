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
std::tuple<float, float, float> Rasterizer::compute_barycentric_2d(float x, float y, const Vec3f* v) {
	float c1 = (x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * y + v[1].x * v[2].y - v[2].x * v[1].y) / (v[0].x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * v[0].y + v[1].x * v[2].y - v[2].x * v[1].y);
	float c2 = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);
	float c3 = 1.0f - c1 - c2;
	return { c1, c2, c3 };
}

// ==========================================
// 新增：核心 Draw 函数 (Vertex Shader 调度)
// ==========================================
void Rasterizer::draw(IShader& shader, int n_verts) {
	for (int i = 0; i < n_verts; i += 3) {
		Vec4f clip_space_verts[3];

		// 1. 运行 Vertex Shader
		for (int j = 0; j < 3; j++) {
			// j 是三角形内索引 (0,1,2), i+j 是全局顶点索引
			clip_space_verts[j] = shader.vertex(j, i + j);
		}

		// 2. 调用光栅化
		// (这里可以添加 裁剪 Clipping 逻辑)
		draw_triangle(shader, clip_space_verts);
	}
}

void Rasterizer::draw_triangle(IShader& shader, const Vec4f* clips) {
	Vec3f v[3];

	// 1. 视口变换 (Viewport Transform)
	for (int i = 0; i < 3; i++) {
		float inv_w = 1.0f / clips[i].w;
		Vec3f ndc = { clips[i].x * inv_w, clips[i].y * inv_w, clips[i].z * inv_w };

		v[i].x = 0.5f * width * (ndc.x + 1.0f);
		v[i].y = 0.5f * height * (ndc.y + 1.0f);
		v[i].z = ndc.z; // 简单线性 Z，不做透视校正插值
	}

	// 2. 包围盒 (Bounding Box)
	float min_x = std::min({ v[0].x, v[1].x, v[2].x });
	float max_x = std::max({ v[0].x, v[1].x, v[2].x });
	float min_y = std::min({ v[0].y, v[1].y, v[2].y });
	float max_y = std::max({ v[0].y, v[1].y, v[2].y });

	// 限制在屏幕范围内
	min_x = std::max(0.0f, (float)floor(min_x));
	max_x = std::min((float)width - 1, (float)ceil(max_x));
	min_y = std::max(0.0f, (float)floor(min_y));
	max_y = std::min((float)height - 1, (float)ceil(max_y));

	// 3. 遍历像素
	for (int y = (int)min_y; y <= (int)max_y; y++) {
		for (int x = (int)min_x; x <= (int)max_x; x++) {

			// ==========================================
			// RGSS 4x 采样循环
			// ==========================================
			for (int k = 0; k < 4; k++) {

				// 3.1 计算当前采样点的精确屏幕坐标
				// 不再是 x + 0.5, 而是加上偏移量
				float sx = (float)x + rgss_offsets[k][0];
				float sy = (float)y + rgss_offsets[k][1];

				// 3.2 针对该采样点计算重心坐标
				auto [alpha, beta, gamma] = compute_barycentric_2d(sx, sy, v);

				// 3.3 覆盖测试 (Inside Test)
				if (alpha >= 0 && beta >= 0 && gamma >= 0) {

					// 3.4 深度插值 (Z-Interpolation)
					float z_interpolated = alpha * v[0].z + beta * v[1].z + gamma * v[2].z;

					// 3.5 深度测试 (Z-Test)
					// 获取当前像素的第 k 个采样点的索引
					// 注意：get_index 返回像素块起始位置，我们需要 +k 访问子采样点
					int idx = get_index(x, y) + k;

					if (z_interpolated < depth_buffer[idx]) {

						// 3.6 调用 Shader (SSAA: 每个采样点都计算颜色)
						// 这会带来更精确的纹理采样和光照
						Vec3f pixel_color = shader.fragment(alpha, beta, gamma);

						// 3.7 写入子采样缓冲区
						frame_buffer[idx] = pixel_color;
						depth_buffer[idx] = z_interpolated;
					}
				}
			} // end sample loop

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