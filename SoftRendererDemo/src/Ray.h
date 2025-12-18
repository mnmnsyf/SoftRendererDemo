#pragma once
#include "GMath.h"

// ========================================================================
// 光线类 (Ray)
// ========================================================================
struct Ray {
	Vec3f orig;     // 光线起点 (Origin)
	Vec3f dir;      // 光线方向 (Direction)
	Vec3f invDir;   // 方向的倒数 (1/dir)

	// 构造函数
	Ray(const Vec3f& origin, const Vec3f& direction)
		: orig(origin), dir(direction) {
		invDir = Vec3f(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
	}

	// 获取光线在参数 t 处的点坐标: P(t) = origin + t * direction
	Vec3f pointAt(float t) const {
		return orig + dir * t;
	}
};

// ========================================================================
// 击中记录
// ========================================================================
struct HitRecord {
	float t;        // 击中距离
	Vec3f p;        // 击中点坐标
	Vec3f normal;   // 击中点法线
	bool front_face;// 光线是从外部击中还是内部击中？

	// 辅助函数：统一法线方向
	// 确保法线始终指向光线射来的一侧
	void set_face_normal(const Ray& r, const Vec3f& outward_normal) {
		front_face = r.dir.dot(outward_normal) < 0;
		normal = front_face ? outward_normal : outward_normal * -1.0f;
	}
};