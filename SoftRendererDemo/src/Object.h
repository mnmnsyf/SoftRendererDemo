#pragma once
#include "Ray.h"
#include <vector>
#include <memory>

// AABB (轴对齐包围盒)
struct AABB {
	Vec3f min;
	Vec3f max;
	AABB() : min(Vec3f(FLT_MAX)), max(Vec3f(-FLT_MAX)) {}
	AABB(const Vec3f& minVal, const Vec3f& maxVal) : min(minVal), max(maxVal) {}

	// 核心功能：Slab Method 相交测试
	bool intersect(const Ray& r, float tmin, float tmax) const;

	// 扩展包围盒
	void expand(const Vec3f& p);
	void expand(const AABB& other);

	// 获取中心点
	Vec3f center() const { return (min + max) * 0.5f; }

	// 获取最长轴的索引 (0=x, 1=y, 2=z)
	int max_extent_axis() const;

	// 表面积
	float surface_area() const;
};

// 抽象基类
class Object {
public:
	virtual ~Object() = default;

	virtual bool intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const = 0;
	virtual AABB get_bounding_box() const = 0;
};
