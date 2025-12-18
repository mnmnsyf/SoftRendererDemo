#include "Primitives.h"

// ========================================================================
// Sphere 实现
// ========================================================================
bool Sphere::intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const {
	//几何法
	Vec3f L = center - r.orig;	// 从光线起点 O 指向球心 center 的向量 L
	float tca = L.dot(r.dir);	// 计算向量 L 在光线方向 D 上的投影长度。我们把这个投影点称为“最近点”，因为在光这条直线上，这个点离球心最近
	float d2 = L.dot(L) - tca * tca; // 直角三角形：斜边是 L，一条直角边是 tca，另一条直角边就是球心到光线的垂直距离 d。
	float r2 = radius * radius;

	if (d2 > r2) return false; // 判断是否相交, 比较垂直距离 d 和球半径 r

	float thc = std::sqrt(r2 - d2); // 计算“半弦长”d^2 + t_{hc}^2 = r^2
	float t0 = tca - thc; // 最终的交点距离 t 就是投影距离 tca 加上或减去半弦长 thc
	float t1 = tca + thc;

	// 找到最近的有效交点
	float temp_t = t0;
	if (temp_t < tmin || temp_t > tmax) {
		temp_t = t1;
		if (temp_t < tmin || temp_t > tmax) {
			return false;
		}
	}

	// 记录击中信息
	rec.t = temp_t;
	rec.p = r.pointAt(rec.t);
	// 球面法线 = (击中点 - 球心) / 半径
	rec.normal = (rec.p - center) / radius;

	return true;
}

AABB Sphere::get_bounding_box() const {
	// 球的包围盒很简单：中心 +/- 半径
	Vec3f rVec(radius, radius, radius);
	return AABB(center - rVec, center + rVec);
}

// ========================================================================
// Triangle 实现
// ========================================================================
bool Triangle::intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const
{
	const float EPSILON = 1e-6f;
	Vec3f edge1 = v1 - v0;
	Vec3f edge2 = v2 - v0;
	Vec3f h = r.dir.cross(edge2);
	float a = edge1.dot(h);

	// 如果 a 接近 0，说明光线平行于三角形平面，无法相交
	if (a > -EPSILON && a < EPSILON) return false;

	float f = 1.0f / a;
	Vec3f s = r.orig - v0;
	float u = f * s.dot(h);

	// 检查 u 的范围
	if (u < 0.0f || u > 1.0f) return false;

	Vec3f q = s.cross(edge1);
	float v = f * r.dir.dot(q);

	// 检查 v 的范围
	if (v < 0.0f || u + v > 1.0f) return false;

	// 计算 t
	float t = f * edge2.dot(q);

	if (t < tmin || t > tmax) return false;

	rec.t = t;
	rec.p = r.pointAt(t);
	rec.set_face_normal(r, normal);

	return true;
}

AABB Triangle::get_bounding_box() const {
	AABB box;
	box.expand(v0);
	box.expand(v1);
	box.expand(v2);
	return box;
}

Vec3f Triangle::get_centroid() const {
	return (v0 + v1 + v2) / 3.0f;
}
