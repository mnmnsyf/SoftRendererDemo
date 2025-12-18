#pragma once
#include "Rasterizer.h"
#include "Scene.h"
#include "Camera.h"

//负责循环像素和着色
class RayTracer {
public:
	RayTracer(Rasterizer* rst, Scene* scn, OrbitCamera* cam)
		: rasterizer(rst), scene(scn), camera(cam) {
	}

	void render();

private:
	Vec3f trace(const Ray& r) {
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

	Rasterizer* rasterizer;
	Scene* scene;
	OrbitCamera* camera;
};