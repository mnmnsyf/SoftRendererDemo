#include "TestCC.h"
#include <iostream>
#include "Rasterizer.h"
#include "Texture.h"
#include "RenderUtils.h"
#include "Model.h"
#include "Camera.h"
#include <iomanip>  // 用于生成文件名 padding (001.ppm)
#include <sstream>

// ==========================================
// 验证测试：Flat vs Gouraud vs Phong
// ==========================================
void TestCC::run_shading_test() {
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
void TestCC::run_specular_comparison() {
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
void TestCC::run_rainbow_triangle_demo() {
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
void TestCC::run_z_buffer_test() {
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

// ==========================================
// demo：纹理调制 
// ==========================================
void TestCC::run_texture_test() {
	std::cout << "Running Texture Modulation" << std::endl;

	const int width = 800;
	const int height = 600;
	Rasterizer r(width, height);

	// --- 1. 准备纹理 ---
	Texture checker_tex;
	checker_tex.setScale(10.0f); // 10x10 的格子
	checker_tex.setColors(Vec3f(1.0f, 1.0f, 1.0f), Vec3f(0.1f, 0.1f, 0.1f)); // 白/黑格

	// --- 2. 准备 Shader ---
	BlinnPhongShader shader;
	// 变换矩阵
	Vec3f eye(0, 0, 3.0f);
	Vec3f center(0, 0, 0);
	Vec3f up(0, 1, 0);
	shader.view = Mat4::lookAt(eye, center, up);
	shader.projection = Mat4::perspective(45.0f, (float)width / height, 0.1f, 100.0f);
	shader.model = Mat4::identity();
	shader.camera_pos = eye;

	// --- 3. 设置光照 (配合物理衰减 1/r^2) ---
	// 光源位置放在右上方
	shader.light.position = Vec3f(2.0f, 2.0f, 2.0f);

	// 物理衰减非常快，需要极高的强度才能照亮物体
	// 距离约 sqrt(2^2+2^2+2^2) = sqrt(12) ≈ 3.46
	// 距离平方 ≈ 12
	// 如果想要表面亮度约为 1.0，强度至少设为 12 以上，考虑漫反射角度，设为 50~100 比较合适
	shader.light.intensity = Vec3f(80.0f, 80.0f, 80.0f);

	// --- 4. 设置材质与纹理 ---
	shader.texture = &checker_tex;
	shader.use_texture = true;
	shader.p = 150.0f; // 高光锐度

	// 演示“调制(Modulation)”效果：
	// 我们把材质底色 k_d 设为红色。
	// 预期结果：白格子变红，黑格子依然黑（红色 * 白色 = 红色）。
	shader.k_d = Vec3f(1.0f, 0.2f, 0.2f);
	shader.k_a = Vec3f(0.01f, 0.01f, 0.01f); // 环境光很弱
	shader.k_s = Vec3f(1.0f, 1.0f, 1.0f);    // 白光高光

	// --- 5. 生成并传递几何数据 ---
	Mesh sphere = generate_sphere(1.0f, 40, 40);

	// 将索引数据展开传给 Shader
	bind_mesh_to_shader(sphere, shader);

	// --- 6. 绘制 ---
	r.clear(Vec3f(0, 0, 0)); // 清除背景为黑色

	// 注意：draw 接受的是顶点数量
	r.draw(shader, shader.in_positions.size());

	r.save_to_ppm("texture_modulation_test.ppm");
	std::cout << "Done. Saved to texture_modulation_test.ppm" << std::endl;
}

// ==========================================
// demo:集成测试函数
// ==========================================
void TestCC::run_integrated_test() {
	std::cout << "Starting Integrated Test..." << std::endl;

	const int width = 800;
	const int height = 600;
	Rasterizer r(width, height);

	// --- 准备资源 ---
	Mesh quad = generate_quad();
	Texture tex;

	// 配置纹理参数
	// A. 这里的 scale 决定了棋盘格的密度
	tex.setScale(10.0f);
	// B. 创建一个极低分辨率的 Buffer (16x16)，用于测试 Bilinear 的模糊效果
	tex.createTestPattern(16, 16);

	// --- 准备 Shader ---
	BlinnPhongShader shader;
	shader.texture = &tex;
	shader.use_texture = true;

	// 设置高环境光，让纹理看得更清楚，不受光照角度影响太深
	shader.k_a = Vec3f(0.8f, 0.8f, 0.8f);
	shader.k_d = Vec3f(0.2f, 0.2f, 0.2f);
	shader.light.position = Vec3f(0, 0, 10);
	shader.light.intensity = Vec3f(10, 10, 10);

	// --- 变换矩阵 setup (制造透视) ---
	// 1. Camera: 放在高处
	Vec3f eye(0, 0.5f, 2.5f);
	Vec3f center(0, 0, 0);
	Vec3f up(0, 1, 0);
	shader.view = Mat4::lookAt(eye, center, up);
	shader.projection = Mat4::perspective(45.0f, (float)width / height, 0.1f, 100.0f);

	// 2. Model: 将正方形放倒并旋转，产生透视纵深感
	// 先平移到 z=-1，并绕 X 转 -60度

	Mat4 translation = Mat4::translate(0, 0, -1.0f);
	Mat4 rotation = Mat4::rotateX(-60) * Mat4::rotateY(30);

	shader.model = translation * rotation;
	shader.camera_pos = eye;

	// 传递顶点数据
	bind_mesh_to_shader(quad, shader);

	// ==========================================
	// 测试 1: 透视矫正 (Checkerboard)
	// ==========================================
	std::cout << "Rendering Pass 1: Perspective Correction Check..." << std::endl;
	shader.sample_mode = BlinnPhongShader::MODE_CHECKERBOARD; // <--- 切换模式

	r.clear(Vec3f(0.5f, 0.7f, 0.9f)); // 天空蓝背景
	r.draw(shader, shader.in_positions.size());
	r.save_to_ppm("test_01_perspective.ppm");

	// ==========================================
	// 测试 2: 双线性插值 (Bilinear)
	// ==========================================
	std::cout << "Rendering Pass 2: Bilinear Interpolation Check..." << std::endl;
	shader.sample_mode = BlinnPhongShader::MODE_BILINEAR; // <--- 切换模式

	r.clear(Vec3f(0.5f, 0.7f, 0.9f)); // 天空蓝背景
	r.draw(shader, shader.in_positions.size());
	r.save_to_ppm("test_02_bilinear.ppm");
}


// ==========================================
// demo:scene_image_texture
// ==========================================
void TestCC::scene_image_texture_test() {
	std::cout << "[Test] Image Texture Loading..." << std::endl;
	Rasterizer r(800, 600);

	// 1. 创建纹理并加载图片
	Texture tex;
	// 尝试加载图片 (请确保文件存在!)
	if (!tex.loadTexture("assets/textures/emoji.png")) {
		std::cerr << "Error: Could not load emoji.png. Make sure the file is in the working directory." << std::endl;
		// 如果失败，生成一个测试图兜底
		tex.createTestPattern(64, 64);
	}
	tex.setScale(10.0f);

	// 2. 创建 Mesh
	Mesh quad = generate_quad();

	// 3. 配置 Shader
	BlinnPhongShader shader;
	setup_base_shader(shader, 800, 600);

	Mat4 translation = Mat4::translate(0, 0, -1.0f);
	Mat4 rotation = Mat4::rotateX(-60);

	shader.model = translation * rotation;
	shader.view = Mat4::lookAt(Vec3f(0, 0, 3), Vec3f(0, 0, 0), Vec3f(0, 1, 0));
	shader.camera_pos = Vec3f(0, 0, 3);

	shader.texture = &tex;
	shader.use_texture = true;
	shader.sample_mode = BlinnPhongShader::MODE_BILINEAR;

	// 4. 绘制
	bind_mesh_to_shader(quad, shader);
	r.clear(Vec3f(0.5f, 0.7f, 0.9f));
	r.draw(shader, shader.in_positions.size());
	r.save_to_ppm("output_image_texture.ppm");
}

void TestCC::normalize_mesh(Mesh& mesh) {
	if (mesh.positions.empty()) return;

	// 1. 计算包围盒
	Vec3f min_box(1e9, 1e9, 1e9);
	Vec3f max_box(-1e9, -1e9, -1e9);

	for (const auto& p : mesh.positions) {
		if (p.x < min_box.x) min_box.x = p.x;
		if (p.y < min_box.y) min_box.y = p.y;
		if (p.z < min_box.z) min_box.z = p.z;

		if (p.x > max_box.x) max_box.x = p.x;
		if (p.y > max_box.y) max_box.y = p.y;
		if (p.z > max_box.z) max_box.z = p.z;
	}

	// 2. 计算中心点和最大跨度
	Vec3f center = (max_box + min_box) * 0.5f;
	float max_dim = std::max(max_box.x - min_box.x, std::max(max_box.y - min_box.y, max_box.z - min_box.z));

	// 3. 归一化所有顶点
	for (auto& p : mesh.positions) {
		p = (p - center) * (2.0f / max_dim); // 缩放到 [-1, 1]
	}

	std::cout << "Mesh Normalized. Center moved from " << center.x << "," << center.y << " to 0,0" << std::endl;
}

void TestCC::run_model_loading_test() {
	// 1. 加载模型
	Model model("assets/models/model.obj");
	Mesh mesh = model.get_mesh();
	normalize_mesh(mesh);

	// 2. 加载对应的纹理
	Texture tex;
	tex.loadTexture("assets/models/texture.png");
	tex.setScale(1.0f);

	// 3. 配置 Shader
	BlinnPhongShader shader;
	setup_base_shader(shader, 800, 600);
	shader.texture = &tex;
	shader.use_texture = true;

	// 调整模型位置
	// 只需要平移，不需要缩放了，因为我们已经归一化了
	shader.model = Mat4::translate(0, 0, -3.0f); // 往 Z 轴深处推 3 米
	// 配合 LookAt
	shader.view = Mat4::lookAt(Vec3f(0, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 0));

	bind_mesh_to_shader(mesh, shader);

	Rasterizer r(800, 600);
	r.clear(Vec3f(0.1f, 0.1f, 0.1f));
	r.draw(shader, shader.in_positions.size());
	r.save_to_ppm("obj_test.ppm");
}

void TestCC::run_turntable_animation() {
	std::cout << "Rendering Turntable Animation..." << std::endl;

	// 1. 加载资源
	Rasterizer r(800, 600);
	Model model("assets/models/model.obj");
	Mesh mesh = model.get_mesh();

	normalize_mesh(mesh);

	Texture tex;
	tex.loadTexture("assets/models/texture.png");

	BlinnPhongShader shader;
	setup_base_shader(shader, 800, 600);
	shader.texture = &tex;
	shader.use_texture = true;

	// 2. 初始化摄像机
	// 目标看向原点，距离 2.5 米
	OrbitCamera camera(Vec3f(0, 0, 0), 2.5f);

	//稍微抬高一点视角 (Pitch)
	camera.phi = 0.3f;

	// 3. 渲染循环 (生成 36 帧)
	int total_frames = 36;
	for (int i = 0; i < total_frames; ++i) {
		r.clear(Vec3f(0.1f, 0.1f, 0.1f));

		// --- 核心：更新 View Matrix ---
		shader.view = camera.get_view_matrix();

		// 绘制
		bind_mesh_to_shader(mesh, shader);
		r.draw(shader, shader.in_positions.size());

		// 保存文件 (frame_000.ppm, frame_001.ppm ...)
		std::stringstream ss;
		ss << "output/frame_" << std::setw(3) << std::setfill('0') << i << ".ppm";
		r.save_to_ppm(ss.str().c_str());

		std::cout << "Rendered frame " << i << "/" << total_frames << "\r";

		// --- 核心：移动摄像机 ---
		// 每帧转 10 度 (2*PI / 36)
		camera.orbit(2.0f * 3.14159f / 36.0f, 0.0f);
	}
	std::cout << "\nDone!" << std::endl;
}