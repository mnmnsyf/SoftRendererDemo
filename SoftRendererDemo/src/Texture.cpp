// --- 引入 stb_image ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include <iostream>
#include <cmath>
#include <algorithm>


Texture::Texture()
	: colorA(1.0f, 1.0f, 1.0f), colorB(0.0f, 0.0f, 0.0f), scale(10.0f) {
}

// ==========================================
// 核心：加载纹理
// ==========================================
bool Texture::loadTexture(const std::string& path) {
	int w, h, channels;

	// 1. 设置翻转 (OpenGL 习惯 UV 原点在左下，图片通常在左上)
	// 开启这个可以让图片方向正确
	stbi_set_flip_vertically_on_load(true);

	// 2. 加载图片
	// 最后一个参数 3 强制要求加载为 RGB (忽略 Alpha 通道，或者把 Grey 转为 RGB)
	unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 3);

	if (!data) {
		std::cerr << "Failed to load texture: " << path << std::endl;
		return false;
	}

	// 3. 更新尺寸
	width = w;
	height = h;
	buffer.resize(w * h);

	// 4. 填充 Buffer (转换 unsigned char [0-255] -> float [0.0-1.0])
	for (int i = 0; i < w * h; ++i) {
		// data 里的排列是 R, G, B, R, G, B ...
		float r = data[i * 3 + 0] / 255.0f;
		float g = data[i * 3 + 1] / 255.0f;
		float b = data[i * 3 + 2] / 255.0f;

		buffer[i] = Vec3f(r, g, b);
	}

	// 5. 释放 stb 内存
	stbi_image_free(data);

	std::cout << "Texture loaded: " << path << " (" << w << "x" << h << ")" << std::endl;
	return true;
}

// ==========================================
// 通用采样接口
// ==========================================
Vec3f Texture::sample(float u, float v) const {
	// 如果 Buffer 有数据，就用双线性插值采样图片
	if (!buffer.empty()) {
		return getColorBilinear(u, v);
	}
	return getColorCheckerboard(u, v);
}

void Texture::setColors(const Vec3f& c1, const Vec3f& c2) {
	colorA = c1;
	colorB = c2;
}

void Texture::setScale(float s) {
	scale = s;
}

Vec3f Texture::getColorCheckerboard(float u, float v) const {
	// 1. 缩放 UV
	float u_scaled = u * scale;
	float v_scaled = v * scale;

	// 2. 使用 floor 向下取整，确保负数区间正确处理
	int x = (int)std::floor(u_scaled);
	int y = (int)std::floor(v_scaled);

	// 3. 奇偶校验
	// C++ 中负数取模结果可能为负 (例如 -1 % 2 = -1)，但 != 0 依然成立，所以逻辑是通用的
	if ((x + y) % 2 == 0) {
		return colorA;
	}
	else {
		return colorB;
	}
}

// ==========================================
// 辅助：处理边界 (Repeat 模式)
// ==========================================
Vec3f Texture::getTexel(int x, int y) const {
	if (width == 0 || height == 0) return { 0, 0, 0 };

	// 实现 Repeat (Wrap) 模式
	// 保证坐标在 [0, width-1] 和 [0, height-1] 范围内
	int x_wrap = ((x % width) + width) % width;
	int y_wrap = ((y % height) + height) % height;

	// Clamp to Edge 模式(解决黑边问题)
	/*int x_clamp = std::max(0, std::min(x, width - 1));
	int y_clamp = std::max(0, std::min(y, height - 1));*/

	// 获取 Buffer 索引
	int index = y_wrap * width + x_wrap;
	//int index = y_clamp * width + x_clamp;
	return buffer[index];
}

// ==========================================
// 核心：双线性插值 (Bilinear Interpolation)
// ==========================================
Vec3f Texture::getColorBilinear(float u, float v) const {
	// 如果没有 buffer 数据，回退到默认颜色或程序化纹理
	if (buffer.empty()) return getColorCheckerboard(u, v);

	// ==========================================
	// 1. 处理 UV 平铺 (Tiling / Repeat)
	// ==========================================
	// 先放大 UV (应用 scale)
	float u_scaled = u * scale;
	float v_scaled = v * scale;

	// 取小数部分 (Fract)，让坐标永远循环在 [0, 1) 之间
	// 例如：u=1.5 -> 0.5, u=2.1 -> 0.1
	float u_img = u_scaled - std::floor(u_scaled);
	float v_img = v_scaled - std::floor(v_scaled);

	// ==========================================
	// 2. 映射到纹理尺寸
	// ==========================================
	float x_prime = u_img * width;
	float y_prime = v_img * height;

	// 2. 找到左下角整数坐标 (x0, y0)
	// std::floor 确保负数也能正确向下取整 (e.g. -0.5 -> -1)
	int x0 = (int)std::floor(x_prime - 0.5f);
	int y0 = (int)std::floor(y_prime - 0.5f);

	// 3. 找到其他 3 个邻居
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	// 4. 计算权重 (s, t) -> 也就是小数部分
	// 这里的中心点对齐修正：s = (u * w) - (x0 + 0.5)
	float s = (x_prime - 0.5f) - std::floor(x_prime - 0.5f);
	float t = (y_prime - 0.5f) - std::floor(y_prime - 0.5f);

	// 5. 获取 4 个 Texel 的颜色
	// c00 -- c10
	//  |      |
	// c01 -- c11  (注意：通常图片数据 y 向下增加还是向上增加取决于你的坐标系)
	// 这里假设 y 向下增加:
	// (x0, y0) (x1, y0)
	// (x0, y1) (x1, y1)

	Vec3f c00 = getTexel(x0, y0); // 左上
	Vec3f c10 = getTexel(x1, y0); // 右上
	Vec3f c01 = getTexel(x0, y1); // 左下
	Vec3f c11 = getTexel(x1, y1); // 右下

	// 6. 三次 Lerp (线性插值)
	// 水平插值 1 (Top)
	Vec3f c_top = c00 * (1.0f - s) + c10 * s;

	// 水平插值 2 (Bottom)
	Vec3f c_bot = c01 * (1.0f - s) + c11 * s;

	// 垂直插值 (Final)
	Vec3f c_final = c_top * (1.0f - t) + c_bot * t;

	return c_final;
}

// ==========================================
// 辅助：生成一个低分辨率测试图
// ==========================================
void Texture::createTestPattern(int w, int h) {
	width = w;
	height = h;
	buffer.resize(w * h);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			// 生成一个简单的彩色格子图案
			int index = y * w + x;

			// 8x8 的格子
			bool is_white = ((x / 8) + (y / 8)) % 2 == 0;

			if (is_white) {
				// 白色区域放一点渐变，方便观察插值
				//float r = (float)x / w;
				//buffer[index] = Vec3f(r, 0.2f, 0.2f); // 红色渐变
				buffer[index] = Vec3f(1.0f, 1.0f, 1.0f); // 白
			}
			else {
				float b = (float)y / h;
				//buffer[index] = Vec3f(0.2f, 0.2f, b); // 蓝色渐变
				buffer[index] = Vec3f(0.0f, 0.0f, 0.0f); // 黑
			}
		}
	}
}