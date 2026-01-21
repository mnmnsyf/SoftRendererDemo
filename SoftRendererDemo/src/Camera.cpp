#include "Camera.h"
#include "Ray.h"

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

Ray OrbitCamera::get_ray(float s, float t) const
{
	// 1. 计算相机坐标系 (UVN)
	// 注意：这里要跟 get_view_matrix 的逻辑保持一致，确保两个模式下看到的一样
	// get_view_matrix 实际上构建的是 World-to-View，我们需要 View-to-World 的轴
	float clamped_phi = std::max(-1.5f, std::min(1.5f, phi));
	float x = radius * std::cos(clamped_phi) * std::sin(theta);
	float y = radius * std::sin(clamped_phi);
	float z = radius * std::cos(clamped_phi) * std::cos(theta);

	Vec3f eye = target + Vec3f(x, y, z);
	Vec3f forward = (target - eye).normalize(); // -w
	Vec3f world_up(0, 1, 0);
	Vec3f right = forward.cross(world_up).normalize(); // u
	Vec3f up = right.cross(forward).normalize(); // v

	// 2. 计算画布参数
	float theta_rad = fov * 3.14159f / 180.0f;
	float half_height = std::tan(theta_rad / 2.0f);
	float half_width = aspect * half_height;

	// 3. 计算光线方向 (这是简化的针孔模型)
	// 屏幕中心是 eye + forward
	// 屏幕左下角
	Vec3f lower_left_corner = eye + forward - right * half_width - up * half_height;
	Vec3f horizontal = right * (2.0f * half_width);
	Vec3f vertical = up * (2.0f * half_height);

	// 生成射线 Dir = lower_left + u*hori + v*vert - origin
	// 利用输入的纹理坐标 (s, t)（范围 0~1）在屏幕上进行线性插值，找到目标像素的世界坐标
	Vec3f dir = lower_left_corner + horizontal * s + vertical * t - eye;
	return Ray(eye, dir.normalize());
}

