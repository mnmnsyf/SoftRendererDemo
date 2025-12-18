#include "Object.h"

// ========================================================================
// AABB 实现
// ========================================================================
// Slab Method 光线相交测试
bool AABB::intersect(const Ray& r, float tmin, float tmax) const {
	for (int i = 0; i < 3; ++i) {
		float origin = r.orig[i];
		float invD = r.invDir[i];
		float minVal = min[i];
		float maxVal = max[i];

		float t0 = (minVal - origin) * invD;
		float t1 = (maxVal - origin) * invD;

		if (invD < 0.0f) std::swap(t0, t1);

		tmin = std::max(tmin, t0);
		tmax = std::min(tmax, t1);

		if (tmin > tmax) return false;
	}
	return true;
}

void AABB::expand(const Vec3f& p) {
	min.x = std::min(min.x, p.x);
	min.y = std::min(min.y, p.y);
	min.z = std::min(min.z, p.z);

	max.x = std::max(max.x, p.x);
	max.y = std::max(max.y, p.y);
	max.z = std::max(max.z, p.z);
}

void AABB::expand(const AABB& other) {
	min.x = std::min(min.x, other.min.x);
	min.y = std::min(min.y, other.min.y);
	min.z = std::min(min.z, other.min.z);

	max.x = std::max(max.x, other.max.x);
	max.y = std::max(max.y, other.max.y);
	max.z = std::max(max.z, other.max.z);
}

int AABB::max_extent_axis() const {
	Vec3f d = max - min;
	if (d.x > d.y && d.x > d.z) return 0; // X轴最长
	if (d.y > d.z) return 1;              // Y轴最长
	return 2;                             // Z轴最长
}

float AABB::surface_area() const {
	Vec3f d = max - min;
	return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
}