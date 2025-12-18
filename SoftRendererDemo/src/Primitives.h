#pragma once
#include "Object.h"

// ========================================================================
// 球体 (Sphere)
// ========================================================================
class Sphere : public Object {
public:
	Vec3f center;
	float radius;

	Sphere(const Vec3f& c, float r) : center(c), radius(r) {}

	virtual bool intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const override;
	virtual AABB get_bounding_box() const override;
};

// ========================================================================
// 三角形 (Triangle)
// ========================================================================
struct Triangle : public Object {
	Vec3f v0, v1, v2;
	Vec3f normal;
	// 可选：存储指向原始 Mesh 的索引，以便后续插值 UV 和法线
	// int mesh_index; 
	// int face_index;

	Triangle(const Vec3f& _v0, const Vec3f& _v1, const Vec3f& _v2)
		: v0(_v0), v1(_v1), v2(_v2) {
		Vec3f e1 = v1 - v0;
		Vec3f e2 = v2 - v0;
		normal = e1.cross(e2).normalize();
	}

	virtual bool intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const override;
	virtual AABB get_bounding_box() const override;

	// 获取三角形重心
	Vec3f get_centroid() const;
};