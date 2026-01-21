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
	Vec3f trace(const Ray& r);

	Rasterizer* rasterizer;
	Scene* scene;
	OrbitCamera* camera;
};