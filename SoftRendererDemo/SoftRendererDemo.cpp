#pragma once
#include <iostream>
#include "Math.h"
#include <cmath>
#include "Rasterizer.h"
#include "Shader.h"

// ==========================================
// 辅助：简单的 Mesh 结构
// ==========================================
struct Mesh {
	std::vector<Vec3f> positions;
	std::vector<Vec3f> normals;
	std::vector<int> indices; // 可选，如果不使用索引绘制，需要展平
};

// ==========================================
// 辅助：生成球体数据
// radius: 半径
// slices, stacks: 细分程度 (比如 20, 20)
// use_flat_normals: 
//    true  -> 生成“面法线” (Flat Shading 效果，顶点不共用，法线垂直于面)
//    false -> 生成“顶点法线” (Smooth/Phong 效果，顶点共用，法线指向外)
// ==========================================
Mesh generate_sphere(float radius, int slices, int stacks, bool use_flat_normals) {
	Mesh mesh;
	float pi = 3.14159265359f;

	if (use_flat_normals) {
		// --- 模式 A: Flat Shading (独立三角形，硬棱角) ---
		// 这种模式下，我们直接生成互不相连的三角形
		for (int i = 0; i < slices; ++i) {
			for (int j = 0; j < stacks; ++j) {
				// 计算一个 Quad 的 4 个角点 (theta: 经度, phi: 纬度)
				// 确定球面上任何一个点，只需要知道三个数：半径(Radius, r)：球有多大。
				// 经度(Longitude, theta)：水平方向的角度，转一圈是 0 到 2pi(360度)。
				// 纬度(Latitude, phi)：垂直方向的角度，从北极到南极是 0 到 pi(180度)。
				float theta1 = (float)i / slices * 2 * pi;
				float theta2 = (float)(i + 1) / slices * 2 * pi;
				float phi1 = (float)j / stacks * pi;
				float phi2 = (float)(j + 1) / stacks * pi;

				// 简单的球面坐标转笛卡尔坐标 lambda
				auto get_pos = [&](float theta, float phi) {
					return Vec3f(
						radius * sin(phi) * cos(theta),
						radius * cos(phi), // Y-up
						radius * sin(phi) * sin(theta)
					);
					};

				Vec3f p0 = get_pos(theta1, phi1);// 当前点
				Vec3f p1 = get_pos(theta2, phi1);// 右边的点
				Vec3f p2 = get_pos(theta1, phi2);// 下边的点
				Vec3f p3 = get_pos(theta2, phi2);// 右下角的点

				// 拆分成两个三角形: T1(p0, p2, p1), T2(p1, p2, p3)
				// 计算面法线 (Face Normal)
				Vec3f n1 = (p1 - p0).cross(p2 - p0).normalize();
				Vec3f n2 = (p3 - p1).cross(p2 - p1).normalize();

				// 存入 T1
				mesh.positions.push_back(p0); mesh.normals.push_back(n1);
				mesh.positions.push_back(p2); mesh.normals.push_back(n1);
				mesh.positions.push_back(p1); mesh.normals.push_back(n1);

				// 存入 T2
				mesh.positions.push_back(p1); mesh.normals.push_back(n2);
				mesh.positions.push_back(p2); mesh.normals.push_back(n2);
				mesh.positions.push_back(p3); mesh.normals.push_back(n2);
			}
		}
	}
	else {
		// --- 模式 B: Smooth Shading (共享顶点，插值法线) ---
		// 这种模式下，法线就是 Vertex -> Pos 的方向
		for (int j = 0; j <= stacks; ++j) {
			float phi = (float)j / stacks * pi;
			for (int i = 0; i <= slices; ++i) {
				float theta = (float)i / slices * 2 * pi;

				float x = sin(phi) * cos(theta);
				float y = cos(phi);
				float z = sin(phi) * sin(theta);

				mesh.positions.push_back(Vec3f(x * radius, y * radius, z * radius));
				mesh.normals.push_back(Vec3f(x, y, z)); // 归一化的法线
			}
		}

		// 生成索引 (EBO) - 这里我们为了 Rasterizer 接口简单，手动展开成纯顶点数组
		// 如果你的 Rasterizer 支持 draw_elements 可以直接用 indices
		// 这里做一个临时的转换：Mesh 结构虽然有 indices 但我们在下面手动展平它
		std::vector<Vec3f> flat_pos;
		std::vector<Vec3f> flat_nor;

		for (int j = 0; j < stacks; ++j) {
			for (int i = 0; i < slices; ++i) {
				int p0 = j * (slices + 1) + i;
				int p1 = p0 + 1;
				int p2 = (j + 1) * (slices + 1) + i;
				int p3 = p2 + 1;

				// T1: p0, p2, p1
				flat_pos.push_back(mesh.positions[p0]); flat_nor.push_back(mesh.normals[p0]);
				flat_pos.push_back(mesh.positions[p2]); flat_nor.push_back(mesh.normals[p2]);
				flat_pos.push_back(mesh.positions[p1]); flat_nor.push_back(mesh.normals[p1]);

				// T2: p1, p2, p3
				flat_pos.push_back(mesh.positions[p1]); flat_nor.push_back(mesh.normals[p1]);
				flat_pos.push_back(mesh.positions[p2]); flat_nor.push_back(mesh.normals[p2]);
				flat_pos.push_back(mesh.positions[p3]); flat_nor.push_back(mesh.normals[p3]);
			}
		}
		mesh.positions = flat_pos;
		mesh.normals = flat_nor;
	}
	return mesh;
}

// ==========================================
// 验证测试：Flat vs Gouraud vs Phong
// ==========================================
void run_shading_test() {
	std::cout << "Running Shading Comparison: Flat vs Gouraud vs Phong..." << std::endl;

	// 加宽画布以容纳 3 个球
	const int width = 1200;
	const int height = 400;
	Rasterizer r(width, height);
	r.clear(Vec3f(0.1f, 0.1f, 0.1f));

	// ------------------------------------------
	// 1. 设置通用参数
	// ------------------------------------------
	Vec3f eye(0, 0, 4.5f); // 稍微离远一点
	Vec3f center(0, 0, 0);
	Vec3f up(0, 1, 0);

	Mat4 view = Mat4::lookAt(eye, center, up);
	Mat4 projection = Mat4::perspective(45.0f, (float)width / height, 0.1f, 50.0f);

	Light common_light;
	common_light.position = { 0.0f, 10.0f, 10.0f };
	common_light.intensity = { 80.0f, 80.0f, 80.0f }; // 确保亮度足够

	// ------------------------------------------
	// 2. 左侧：Flat Shading (面法线)
	// ------------------------------------------
	{
		std::cout << "Draw 1/3: Flat Shading..." << std::endl;
		BlinnPhongShader shader; // 用 Phong Shader 配合 Flat Normal 数据也能出效果
		shader.view = view; shader.projection = projection; shader.light = common_light; shader.camera_pos = eye;
		shader.model = Mat4::translate(-2.5f, 0.0f, 0.0f); // 移到最左边

		shader.k_d = { 0.8f, 0.2f, 0.2f }; // 红
		shader.p = 100.0f;

		Mesh mesh = generate_sphere(1.0f, 20, 20, true); // <--- true: 使用面法线
		shader.in_positions = mesh.positions;
		shader.in_normals = mesh.normals;
		r.draw(shader, mesh.positions.size());
	}

	// ------------------------------------------
	// 3. 中间：Gouraud Shading (顶点着色)
	// ------------------------------------------
	{
		std::cout << "Draw 2/3: Gouraud Shading..." << std::endl;
		GouraudShader shader; // <--- 使用新的 Shader
		shader.view = view; shader.projection = projection; shader.light = common_light; shader.camera_pos = eye;
		shader.model = Mat4::translate(0.0f, 0.0f, 0.0f); // 居中

		shader.k_d = { 0.2f, 0.8f, 0.2f }; // 绿 (方便区分)
		shader.p = 100.0f;

		Mesh mesh = generate_sphere(1.0f, 20, 20, false); // <--- false: 使用平滑法线
		shader.in_positions = mesh.positions;
		shader.in_normals = mesh.normals;
		r.draw(shader, mesh.positions.size());
	}

	// ------------------------------------------
	// 4. 右侧：Phong Shading (片元着色)
	// ------------------------------------------
	{
		std::cout << "Draw 3/3: Phong (Pixel) Shading..." << std::endl;
		BlinnPhongShader shader; // <--- 使用原来的 Shader
		shader.view = view; shader.projection = projection; shader.light = common_light; shader.camera_pos = eye;
		shader.model = Mat4::translate(2.5f, 0.0f, 0.0f); // 移到最右边

		shader.k_d = { 0.2f, 0.2f, 0.8f }; // 蓝 (方便区分)
		shader.p = 100.0f;

		Mesh mesh = generate_sphere(1.0f, 20, 20, false); // <--- false: 使用平滑法线
		shader.in_positions = mesh.positions;
		shader.in_normals = mesh.normals;
		r.draw(shader, mesh.positions.size());
	}

	r.save_to_ppm("shading_comparison.ppm");
}

// ==========================================
// Demo: Classic Phong vs Blinn-Phong
// ==========================================
void run_specular_comparison() {
	std::cout << "Running Specular Comparison: Classic Phong vs Blinn-Phong..." << std::endl;

	const int width = 800;
	const int height = 400;
	Rasterizer r(width, height);
	r.clear(Vec3f(0.1f, 0.1f, 0.1f));

	// 通用设置
	Vec3f eye(0, 0, 4.0f);
	Vec3f center(0, 0, 0);
	Vec3f up(0, 1, 0);
	Mat4 view = Mat4::lookAt(eye, center, up);
	Mat4 projection = Mat4::perspective(45.0f, (float)width / height, 0.1f, 50.0f);

	// 强光
	Light common_light;
	common_light.position = { 0.0f, 10.0f, 10.0f };
	common_light.intensity = { 500.0f, 500.0f, 500.0f };

	// 生成光滑球体模型
	Mesh sphere = generate_sphere(1.0f, 30, 30, false); // false = Smooth Normals

	// ==========================================
	// 1. 左边：Classic Phong
	// ==========================================
	{
		ClassicPhongShader shader;
		shader.view = view; shader.projection = projection; shader.light = common_light; shader.camera_pos = eye;

		shader.model = Mat4::translate(-1.1f, 0.0f, 0.0f); // 左移

		shader.k_d = { 0.8f, 0.2f, 0.2f }; // 红色
		shader.k_s = { 1.0f, 1.0f, 1.0f }; // 白色高光
		shader.p = 64.0f;                  // 高光锐度

		shader.in_positions = sphere.positions;
		shader.in_normals = sphere.normals;
		r.draw(shader, sphere.positions.size());
	}

	// ==========================================
	// 2. 右边：Blinn-Phong
	// ==========================================
	{
		BlinnPhongShader shader; // (假设你之前的 Shader 叫这个名字)
		shader.view = view; shader.projection = projection; shader.light = common_light; shader.camera_pos = eye;

		shader.model = Mat4::translate(1.1f, 0.0f, 0.0f); // 右移

		shader.k_d = { 0.2f, 0.2f, 0.8f }; // 蓝色
		shader.k_s = { 1.0f, 1.0f, 1.0f };
		shader.p = 64.0f;                  // 使用同样的锐度，以便观察区别

		shader.in_positions = sphere.positions;
		shader.in_normals = sphere.normals;
		r.draw(shader, sphere.positions.size());
	}

	r.save_to_ppm("specular_test.ppm");
}

// ==========================================
// Demo 1: 彩虹三角形
// ==========================================
void run_rainbow_triangle_demo() {
	std::cout << "Running Rainbow Triangle Demo..." << std::endl;

	const int width = 800;
	const int height = 600;
	Rasterizer r(width, height);

	// 1. 准备 Shader
	VertexColorShader shader;

	// 2. 设置相机 (View) 和 透视 (Projection)
	Vec3f eye(0, 0, 3);      // 摄像机在 Z=3
	Vec3f center(0, 0, 0);   // 看向原点
	Vec3f up(0, 1, 0);       // Y 轴向上
	shader.view = Mat4::lookAt(eye, center, up);
	shader.projection = Mat4::perspective(45.0f, (float)width / height, 0.1f, 50.0f);
	shader.model = Mat4::identity();

	// 3. 准备顶点数据 (三角形)
	shader.in_positions = {
		{0.0f, 0.5f, 0.0f},   // 上
		{-0.5f, -0.5f, 0.0f}, // 左下
		{0.5f, -0.5f, 0.0f}   // 右下
	};

	// 4. 准备颜色数据 (RGB)
	shader.in_colors = {
		{1.0f, 0.0f, 0.0f}, // 红
		{0.0f, 1.0f, 0.0f}, // 绿
		{0.0f, 0.0f, 1.0f}  // 蓝
	};

	// 5. 绘制
	r.clear(Vec3f(0.1f, 0.1f, 0.1f)); // 深灰背景
	r.draw(shader, 3); // 画 3 个顶点

	r.save_to_ppm("rainbow_triangle.ppm");
}

// ==========================================
// Demo 2: Z-Buffer 遮挡测试
// ==========================================
void run_z_buffer_test() {
	std::cout << "Running Z-Buffer Test..." << std::endl;

	const int width = 800;
	const int height = 600;
	Rasterizer r(width, height);
	VertexColorShader shader;

	// 设置相机
	Vec3f eye(0, 0, 5);
	Vec3f center(0, 0, 0);
	Vec3f up(0, 1, 0);
	shader.view = Mat4::lookAt(eye, center, up);
	shader.projection = Mat4::perspective(45.0f, (float)width / height, 0.1f, 50.0f);

	// 定义两个三角形的数据

	// 红色三角形 (位置偏左，Z=0)
	std::vector<Vec3f> pos_red = {
		{0.0f, 0.5f, 0.0f}, {-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}
	};
	std::vector<Vec3f> col_red(3, Vec3f(1, 0, 0)); // 全红

	// 蓝色三角形 (位置偏右，Z=-1，更远)
	// 理论上，虽然它在后面，但因为我们开启了深度测试，
	// 无论先画谁，红色都应该遮挡住蓝色的重叠部分。
	std::vector<Vec3f> pos_blue = {
		{0.2f, 0.5f, -1.0f}, {-0.3f, -0.5f, -1.0f}, {0.7f, -0.5f, -1.0f}
	};
	std::vector<Vec3f> col_blue(3, Vec3f(0, 0, 1)); // 全蓝

	r.clear(Vec3f(0, 0, 0));

	// 1. 画红色 (Near)
	shader.model = Mat4::identity();
	shader.in_positions = pos_red;
	shader.in_colors = col_red;
	r.draw(shader, 3);

	// 2. 画蓝色 (Far)
	shader.model = Mat4::identity(); // 坐标本身已经包含 Z 位移，所以 Model 矩阵不变
	shader.in_positions = pos_blue;
	shader.in_colors = col_blue;
	r.draw(shader, 3);

	r.save_to_ppm("z_test.ppm");
	// 保存深度图以供检查 (越白越远，越黑越近)
	r.save_depth_to_ppm("z_test_depth.ppm");
}

int main() {

	//run_rainbow_triangle_demo();
	//run_z_buffer_test();
	//run_shading_test();
	run_specular_comparison();
    return 0;
}
