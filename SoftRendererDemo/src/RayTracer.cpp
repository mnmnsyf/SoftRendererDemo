#include "RayTracer.h"
#include <iostream>

void RayTracer::render()
{
	const Vec2f ScreenSize = rasterizer->GetScreenSize();
	int width = ScreenSize.x;
	int height = ScreenSize.y;

	for (int j = 0; j < height; ++j) {
		// 可选：打印进度
		if (j % 50 == 0) std::cout << "Rendering line " << j << std::endl;

		for (int i = 0; i < width; ++i) {
			float u = (float)i / (width - 1);
			float v = (float)j / (height - 1);

			// 从相机发射光线
			Ray r = camera->get_ray(u, v);
			Vec3f color = trace(r);

			// 写入 Rasterizer 的 FrameBuffer
			rasterizer->set_pixel(i, j, color);
		}
	}
}

Vec3f RayTracer::trace(const Ray& r)
{
	HitRecord rec;
	if (scene->intersect(r, rec)) {
		// === 着色逻辑 (Shading) ===

		// 1. 简单的法线着色 (Ace 的轮廓和形状会很明显)
		Vec3f normal_color = (rec.normal + Vec3f(1, 1, 1)) * 0.5f;
		return normal_color;

		// 2. 或者简单的漫反射 (假定光在相机位置)
		// Vec3f light_dir = (camera->target - camera->get_position()).normalize() * -1; // 假设光
		// float diff = std::max(0.0f, rec.normal.dot(Vec3f(1,1,1).normalize()));
		// return Vec3f(1, 0, 0) * diff; // 红色 Ace
	}

	// 背景色 (天空)
	Vec3f unit_dir = r.dir.normalize();
	float t = 0.5f * (unit_dir.y + 1.0f);
	return Vec3f(1.0f, 1.0f, 1.0f) * (1.0f - t) + Vec3f(0.5f, 0.7f, 1.0f) * t;
}

