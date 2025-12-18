#include "BVH.h"
#include <iostream>

// 比较函数：用于 std::sort
bool box_compare(const Object* a, const Object* b, int axis) {
	AABB box_a = a->get_bounding_box();
	AABB box_b = b->get_bounding_box();
	return box_a.min[axis] < box_b.min[axis];
}

bool box_x_compare(const Object* a, const Object* b) { return box_compare(a, b, 0); }
bool box_y_compare(const Object* a, const Object* b) { return box_compare(a, b, 1); }
bool box_z_compare(const Object* a, const Object* b) { return box_compare(a, b, 2); }

// 递归构造
BVHNode::BVHNode(std::vector<Object*>& src_objects, size_t start, size_t end) {
	// 1. 随机选取一个轴，或者更高级的做法是选包围盒最长的轴
	// 这里为了简单，我们随机选一个轴，或者轮流选
	int axis = rand() % 3;

	// 更好的做法：计算当前这一堆物体的总包围盒，看哪个轴最长
	// 但为了代码简洁，随机轴在简单场景下也够用了

	auto comparator = (axis == 0) ? box_x_compare :
		(axis == 1) ? box_y_compare :
		box_z_compare;

	size_t object_span = end - start;

	if (object_span == 1) {
		// 只有一个物体，左右孩子都指向它
		left = right = src_objects[start];
	}
	else if (object_span == 2) {
		// 只有两个物体，排序后分别赋值
		if (comparator(src_objects[start], src_objects[start + 1])) {
			left = src_objects[start];
			right = src_objects[start + 1];
		}
		else {
			left = src_objects[start + 1];
			right = src_objects[start];
		}
	}
	else {
		// 2. 核心：排序
		std::sort(src_objects.begin() + start, src_objects.begin() + end, comparator);

		// 3. 对半切分
		size_t mid = start + object_span / 2;
		left = new BVHNode(src_objects, start, mid);
		right = new BVHNode(src_objects, mid, end);
	}

	// 4. 计算当前节点的总包围盒
	AABB box_left = left->get_bounding_box();
	AABB box_right = right->get_bounding_box();

	box = AABB(); // 初始化空盒子
	box.expand(box_left);
	box.expand(box_right);
}

// 根构造函数
BVHNode::BVHNode(std::vector<Object*>& objects)
	: BVHNode(objects, 0, objects.size())
{
}

// 核心求交逻辑
bool BVHNode::intersect(const Ray& r, float tmin, float tmax, HitRecord& rec) const {
	// 1. 先检查是否击中大盒子
	if (!box.intersect(r, tmin, tmax))
		return false;

	// 2. 如果击中了大盒子，递归检查左右孩子
	bool hit_left = left->intersect(r, tmin, tmax, rec);

	// 注意：如果左边击中了，tmax 应该更新为 rec.t，这样右边只需要找更近的交点
	// 这里的逻辑稍微有点 trick，为了代码简单，我们传递原始 tmax 或更新后的
	float t_for_right = hit_left ? rec.t : tmax;

	// 这里的 rec 引用会被传递下去。如果右边也击中且更近，rec 会被右边覆盖
	bool hit_right = right->intersect(r, tmin, t_for_right, rec);

	return hit_left || hit_right;
}

AABB BVHNode::get_bounding_box() const {
	return box;
}