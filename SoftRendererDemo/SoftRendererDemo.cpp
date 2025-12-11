#include <iostream>
#include "Math.h"
#include "Rasterizer.h" // 引入光栅化器

// 演示：绘制一个彩虹三角形
void run_rainbow_triangle_demo() {
	// 1. 初始化光栅化器 (800x600 分辨率)
	const int width = 800;
	const int height = 600;
	Rasterizer r(width, height);

	// 2. 准备 MVP 矩阵 (和之前完全一样)
	Vec3f eye(0, 0, 0);
	Vec3f center(0, 0, -1);
	Vec3f up(0, 1, 0);
	Mat4 view = Mat4::lookAt(eye, center, up);
	Mat4 proj = Mat4::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
	Mat4 viewport = Mat4::viewport(width, height);

	// 3. 定义一个三角形 (位于 Z = -5)
	Vec4f v1(0.0f, 0.5f, -2.0f, 1.0f); // 顶
	Vec4f v2(-0.5f, -0.5f, -2.0f, 1.0f); // 左下
	Vec4f v3(0.5f, -0.5f, -2.0f, 1.0f); // 右下
	Vec4f raw_verts[] = { v1, v2, v3 };

	// 【新增】定义三个顶点的颜色 (RGB)
	Vec3f c1(1.0f, 0.0f, 0.0f); // 红色
	Vec3f c2(0.0f, 1.0f, 0.0f); // 绿色
	Vec3f c3(0.0f, 0.0f, 1.0f); // 蓝色
	Vec3f colors[] = { c1, c2, c3 };

	// 用于存储变换后的屏幕坐标
	Vec3f screen_verts[3];

	// 4. 顶点着色器流程 (Vertex Shader Pipeline)
	for (int i = 0; i < 3; i++) {
		// MVP 变换 -> Clip Space
		Vec4f clip = proj * view * raw_verts[i];

		// 透视除法 -> NDC
		// 注意：这里除以 w 非常关键，否则没有近大远小
		Vec3f ndc;
		ndc.x = clip.x / clip.w;
		ndc.y = clip.y / clip.w;
		ndc.z = clip.z / clip.w;

		// 视口变换 -> Screen Space
		Vec4f screen = viewport * Vec4f(ndc.x, ndc.y, ndc.z, 1.0f);

		// 存入数组 (只取 x, y, z)
		screen_verts[i] = Vec3f(screen.x, screen.y, screen.z);
	}

	// 5. 光栅化 (Rasterization)
	// 传入三个屏幕坐标，和一个颜色 (比如绿色)
	r.clear(Vec3f(0.1f, 0.1f, 0.1f)); // 背景设为深灰色
	r.draw_triangle(screen_verts[0], screen_verts[1], screen_verts[2], colors);

	// 6. 保存结果
	r.save_to_ppm("rainbow_triangle.ppm");
}

// 辅助函数：定义一个测试场景
// order_flag: true 先画红(近)，false 先画蓝(远)
void run_z_buffer_test(Rasterizer& r, bool draw_red_first) {
	// 1. 清屏 (颜色和深度缓冲都会被重置)
	r.clear(Vec3f(0, 0, 0));

	// ==========================================
	// 定义三角形数据 (直接使用屏幕坐标 + Z值)
	// 屏幕大小 800x600
	// ==========================================

	// 三角形 1：红色，Z = 0.5 (较近)
	// 这是一个正立的三角形
	Vec3f t1_v0(400, 500, 0.5f); // 上
	Vec3f t1_v1(200, 150, 0.2f); // 左下
	Vec3f t1_v2(600, 150, 0.1f); // 右下
	Vec3f t1_color[3] = { {1, 0, 0}, {1, 0, 0}, {1, 0, 0} }; // 全红

	// 三角形 2：蓝色，Z = 0.9 (较远)
	// 这是一个倒立的三角形，与红色形成交错，便于观察遮挡
	Vec3f t2_v0(400, 100, 0.5f); // 下
	Vec3f t2_v1(200, 450, 0.7f); // 左上
	Vec3f t2_v2(600, 450, 0.9f); // 右上
	Vec3f t2_color[3] = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1} }; // 全蓝

	// ==========================================
	// 执行绘制
	// ==========================================
	if (draw_red_first) {
		std::cout << "测试模式：先画红色(近)，再画蓝色(远)..." << std::endl;

		// 1. 先画红色 (Z=0.5)
		// 此时深度缓冲写入 0.5
		r.draw_triangle(t1_v0, t1_v1, t1_v2, t1_color);

		// 2. 后画蓝色 (Z=0.9)
		// 此时进行深度测试：0.9 < 0.5 ? False -> 蓝色被丢弃，红色保留
		r.draw_triangle(t2_v0, t2_v1, t2_v2, t2_color);

		r.save_to_ppm("z_test_red_first.ppm");
		// 导出深度图
		r.save_depth_to_ppm("depth_map.ppm");
	}
	else {
		std::cout << "测试模式：先画蓝色(远)，再画红色(近)..." << std::endl;

		// 1. 先画蓝色 (Z=0.9)
		// 此时深度缓冲写入 0.9
		r.draw_triangle(t2_v0, t2_v1, t2_v2, t2_color);

		// 2. 后画红色 (Z=0.5)
		// 此时进行深度测试：0.5 < 0.9 ? True -> 红色覆盖蓝色，并更新深度为 0.5
		r.draw_triangle(t1_v0, t1_v1, t1_v2, t1_color);

		r.save_to_ppm("z_test_blue_first.ppm");
	}
}

int main() {

	Rasterizer r(800, 600);

	// 测试 1：先画远(蓝)，后画近(红)
	// 这是“画家算法”也能正确的顺序，作为基准参考
	run_z_buffer_test(r, false);

	// 测试 2：先画近(红)，后画远(蓝)
	// 这是 Z-Buffer 发挥作用的关键时刻！
	// 如果没有 Z-Buffer，蓝色会覆盖红色。
	// 如果有 Z-Buffer，蓝色会被挡住。
	run_z_buffer_test(r, true);

	std::cout << "测试完成！请查看生成的 .ppm 文件。" << std::endl;

    return 0;
}
