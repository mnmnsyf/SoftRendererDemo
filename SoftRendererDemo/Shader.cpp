#include "Shader.h"
#include <algorithm> // for std::max
#include <cmath>     // for std::pow

// ==========================================
// Vertex Shader 实现
// ==========================================
Vec4f BlinnPhongShader::vertex(int iface, int vert_idx) {
	// 1. 读取输入
	// 实际项目中这里要做边界检查
	Vec3f raw_pos = in_positions[vert_idx];
	Vec3f raw_nor = in_normals[vert_idx];

	// 2. 变换法线 -> 世界空间
	// 正确做法是使用 model 的逆转置矩阵 (Inverse Transpose)
	// 这里为了简化，假设只有旋转和平移，直接用 model 矩阵
	Vec4f normal_4 = model * Vec4f(raw_nor, 0.0f); // w=0 代表向量
	varying_normal[iface] = Vec3f(normal_4.x, normal_4.y, normal_4.z);

	// 3. 变换位置 -> 世界空间
	Vec4f world_pos_4 = model * Vec4f(raw_pos, 1.0f); // w=1 代表点
	varying_world_pos[iface] = Vec3f(world_pos_4.x, world_pos_4.y, world_pos_4.z);

	// 4. MVP 变换 -> 裁剪空间 (输出给光栅化器)
	return projection * view * world_pos_4;
}

// ==========================================
// Fragment Shader 实现
// ==========================================
Vec3f BlinnPhongShader::fragment(float alpha, float beta, float gamma) {
	// ------------------------------------------
	// A. 插值并归一化
	// ------------------------------------------
	// 1. 法线插值
	Vec3f normal = varying_normal[0] * alpha + varying_normal[1] * beta + varying_normal[2] * gamma;
	normal = normal.normalize(); // 【关键】插值后的向量长度不为1，必须归一化

	// 2. 世界坐标插值
	Vec3f world_pos = varying_world_pos[0] * alpha + varying_world_pos[1] * beta + varying_world_pos[2] * gamma;

	// ------------------------------------------
	// B. 准备光照向量
	// ------------------------------------------
	// 光源方向 L (从着色点指向光源)
	Vec3f light_vec = light.position - world_pos;
	float dist_sq = light_vec.dot(light_vec); // 距离平方
	Vec3f L = light_vec.normalize();

	// 视线方向 V (从着色点指向摄像机)
	Vec3f V = (camera_pos - world_pos).normalize();

	// 半程向量 H (用于 Blinn-Phong 高光)
	Vec3f H = (L + V).normalize();

	// ------------------------------------------
	// C. 计算 Blinn-Phong 三项
	// ------------------------------------------

	// 0. 光照衰减 (物理上是 1/r^2)
	// 注意：如果光强 intensity 数值较小，距离远了会全黑，需要调大光强
	Vec3f radiance = light.intensity * (1.0f / dist_sq);

	// 1. 环境光 (Ambient) - L_a
	Vec3f ambient = k_a * 0.1f; // 这里的 0.1f 是环境光能量系数

	// 2. 漫反射 (Diffuse) - L_d
	// 公式：Kd * (I/r^2) * max(0, n dot l)
	float diff_factor = std::max(0.0f, normal.dot(L));
	Vec3f diffuse = k_d * radiance * diff_factor;

	// 3. 高光 (Specular) - L_s
	// 公式：Ks * (I/r^2) * max(0, n dot h)^p
	float spec_factor = std::pow(std::max(0.0f, normal.dot(H)), p);
	Vec3f specular = k_s * radiance * spec_factor;

	// ------------------------------------------
	// D. 输出最终颜色
	// ------------------------------------------
	return ambient + diffuse + specular;
}