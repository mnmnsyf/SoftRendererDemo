#include "Camera.h"

Mat4 OrbitCamera::get_view_matrix()
{
	// 1. 球坐标 -> 笛卡尔坐标
		// 限制 phi 防止万向节死锁 (Gimbal Lock) 或翻转
		// 也就是不能完全看到头顶正上方或正下方
	float clamped_phi = std::max(-1.5f, std::min(1.5f, phi));

	float x = radius * std::cos(clamped_phi) * std::sin(theta);
	float y = radius * std::sin(clamped_phi);
	float z = radius * std::cos(clamped_phi) * std::cos(theta);

	Vec3f eye = target + Vec3f(x, y, z);
	Vec3f up = Vec3f(0, 1, 0);

	return Mat4::lookAt(eye, target, up);
}

void OrbitCamera::orbit(float d_theta, float d_phi)
{
	theta += d_theta;
	phi += d_phi;
}

void OrbitCamera::zoom(float d_radius)
{
	radius -= d_radius;
	if (radius < 0.1f) radius = 0.1f; // 别钻到模型里面去
}

