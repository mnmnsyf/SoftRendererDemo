#include "RayTracer.h"
#include <iostream>

void RayTracer::render()
{
	Vec2f ScreenSize = rasterizer->GetScreenSize();
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

