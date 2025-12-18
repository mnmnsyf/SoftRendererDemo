#pragma once
#include "Object.h"
#include <vector>
#include <algorithm>

class BVHNode : public Object {
public:
	// 构建函数：传入一堆物体，构建出一棵树
	BVHNode(std::vector<Object*>& objects);
	// 递归辅助构造函数
	BVHNode(std::vector<Object*>& src_objects, size_t start, size_t end);

	virtual bool intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const override;
	virtual AABB get_bounding_box() const override;

public:
	Object* left;  // 左子树
	Object* right; // 右子树
	AABB box;      // 当前节点的包围盒
};