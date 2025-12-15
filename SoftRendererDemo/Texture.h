#pragma once
#include "Math.h"
#include <vector>
#include <string>

class Texture {
public:
	Texture();

	// 从文件加载
	bool loadTexture(const std::string& path);

	// 采样函数
	Vec3f sample(float u, float v) const;

	// 设置棋盘格颜色
	void setColors(const Vec3f& c1, const Vec3f& c2);
	// 设置缩放（格子密度）
	void setScale(float s);

	// 核心采样函数
	Vec3f getColorCheckerboard(float u, float v) const;


	// 初始化一个简单的测试纹理（比如 64x64 的格子或渐变）
	void createTestPattern(int w, int h);

	// 【核心目标】双线性插值采样
	Vec3f getColorBilinear(float u, float v) const;

public:
	// 图片纹理支持
	int width = 0;
	int height = 0;
	std::vector<Vec3f> buffer; // 存储 r,g,b 数据

private:
	Vec3f colorA;
	Vec3f colorB;
	float scale;

	// 辅助：获取整数坐标的颜色（处理边界/平铺）
	Vec3f getTexel(int x, int y) const;
};