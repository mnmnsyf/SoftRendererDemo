#pragma once
#include "Rasterizer.h" // 包含 IShader 和 Light 的定义
#include "Math.h"
#include <vector>

// BlinnPhongShader：支持环境光、漫反射、高光
struct BlinnPhongShader : public IShader {
	// ==========================================
	// Uniforms (全局变量)
	// ==========================================
	Mat4 model;
	Mat4 view;
	Mat4 projection;

	Vec3f camera_pos; // 观察者位置
	Light light;      // 灯光数据

	// 材质属性
	Vec3f k_a = { 0.1f, 0.1f, 0.1f }; // Ambient
	Vec3f k_d = { 0.8f, 0.8f, 0.8f }; // Diffuse
	Vec3f k_s = { 1.0f, 1.0f, 1.0f }; // Specular
	float p = 150.0f;                 // Shininess (高光指数)

	// ==========================================
	// Attributes (输入数据)
	// ==========================================
	std::vector<Vec3f> in_positions;
	std::vector<Vec3f> in_normals;

	// ==========================================
	// Varyings (插值数据)
	// ==========================================
	Vec3f varying_world_pos[3];
	Vec3f varying_normal[3];

	// ==========================================
	// 接口实现声明 (Override)
	// ==========================================
	virtual Vec4f vertex(int iface, int vert_idx) override;
	virtual Vec3f fragment(float alpha, float beta, float gamma) override;
};

// ==========================================
// 简单的顶点颜色 Shader (用于彩虹三角形和 Z-Buffer 测试)
// ==========================================
struct VertexColorShader : public IShader {
	// Uniforms
	Mat4 model;
	Mat4 view;
	Mat4 projection;

	// Attributes (输入)
	std::vector<Vec3f> in_positions;
	std::vector<Vec3f> in_colors;

	// Varyings (插值)
	Vec3f varying_color[3];

	// 顶点着色器
	virtual Vec4f vertex(int iface, int vert_idx) override {
		// 1. 读取数据
		Vec3f raw_pos = in_positions[vert_idx];
		Vec3f raw_col = in_colors[vert_idx];

		// 2. 传递给 Varying 供插值
		varying_color[iface] = raw_col;

		// 3. MVP 变换 -> 裁剪空间
		return projection * view * model * Vec4f(raw_pos, 1.0f);
	}

	// 片元着色器
	virtual Vec3f fragment(float alpha, float beta, float gamma) override {
		// 简单的颜色线性插值
		return varying_color[0] * alpha + varying_color[1] * beta + varying_color[2] * gamma;
	}
};

struct GouraudShader : public IShader {
	Mat4 model;
	Mat4 view;
	Mat4 projection;
	Vec3f camera_pos;
	Light light;
	Vec3f k_a = { 0.1f, 0.1f, 0.1f };
	Vec3f k_d = { 0.8f, 0.8f, 0.8f };
	Vec3f k_s = { 1.0f, 1.0f, 1.0f };
	float p = 150.0f;

	// Attributes
	std::vector<Vec3f> in_positions;
	std::vector<Vec3f> in_normals;

	// ==========================================
	// Varyings (关键区别！)
	// Gouraud 在顶点算出颜色，所以插值的是颜色
	// ==========================================
	Vec3f varying_color[3];

	virtual Vec4f vertex(int iface, int vert_idx) override;
	virtual Vec3f fragment(float alpha, float beta, float gamma) override;
};

// 经典的 Phong Shader (使用反射向量 R)
struct ClassicPhongShader : public IShader {
	// Uniforms
	Mat4 model;
	Mat4 view;
	Mat4 projection;
	Vec3f camera_pos;
	Light light;
	Vec3f k_a = { 0.1f, 0.1f, 0.1f };
	Vec3f k_d = { 0.8f, 0.8f, 0.8f };
	Vec3f k_s = { 1.0f, 1.0f, 1.0f };
	float p = 38.f;

	// Attributes
	std::vector<Vec3f> in_positions;
	std::vector<Vec3f> in_normals;

	// Varyings
	Vec3f varying_world_pos[3];
	Vec3f varying_normal[3];

	// 接口
	virtual Vec4f vertex(int iface, int vert_idx) override;
	virtual Vec3f fragment(float alpha, float beta, float gamma) override;
};