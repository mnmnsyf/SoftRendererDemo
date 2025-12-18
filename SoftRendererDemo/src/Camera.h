#pragma once
#include "GMath.h"
#include <algorithm>

struct Ray;

class OrbitCamera {
public:
	// 目标点 (模型中心)
	Vec3f target;
	// 摄像机距离目标的距离
	float radius;
	// 水平旋转角度 (Yaw / Azimuth) - 弧度制
	float theta;
	// 垂直旋转角度 (Pitch / Elevation) - 弧度制
	float phi;

	float fov = 45.0f; // 视野角度
	float aspect = 1.33f; // 宽高比

	OrbitCamera(Vec3f t = Vec3f(0, 0, 0), float r = 3.0f)
		: target(t), radius(r), theta(0.0f), phi(0.0f) {
	}

	// 根据当前的 theta, phi, radius 计算 View Matrix
	Mat4 get_view_matrix();

	// 模拟鼠标拖动：水平移动改 theta，垂直移动改 phi
	void orbit(float d_theta, float d_phi);

	// 模拟滚轮缩放
	void zoom(float d_radius);

	// 光线追踪专用：生成光线
	Ray get_ray(float u, float v) const;
};