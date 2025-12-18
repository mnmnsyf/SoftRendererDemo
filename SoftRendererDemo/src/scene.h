#pragma once
#include <vector>
#include "Object.h"
#include "BVH.h"
#include "Model.h"
#include "Primitives.h"

//管理物体的生命周期和 BVH
class Scene {
public:
	~Scene() {
		// 简单的内存清理，或者使用智能指针
		for (auto obj : objects) delete obj;
		if (bvh_root) delete bvh_root;
	}

	void add_object(Object* obj) {
		objects.push_back(obj);
		dirty = true; // 标记需要重建 BVH
	}

	// 将 Model 转换并加入场景
	void add_model(const Model& model) {
		const Mesh& mesh = model.get_mesh();
		for (size_t i = 0; i < mesh.indices.size(); i += 3) {
			int idx0 = mesh.indices[i];
			int idx1 = mesh.indices[i + 1];
			int idx2 = mesh.indices[i + 2];

			Vec3f v0 = mesh.positions[idx0];
			Vec3f v1 = mesh.positions[idx1];
			Vec3f v2 = mesh.positions[idx2];

			// 这里可以把法线也传进去，如果你的 Triangle 支持的话
			add_object(new Triangle(v0, v1, v2));
		}
	}

	void build() {
		if (dirty && !objects.empty()) {
			// 注意：BVHNode 构造函数会重排 objects 向量，所以不要存外部索引
			bvh_root = new BVHNode(objects);
			dirty = false;
		}
	}

	// 场景求交入口
	bool intersect(const Ray& r, HitRecord& rec) const {
		if (!bvh_root) return false;
		// tmin 设为 0.001 防止自我遮挡
		return bvh_root->intersect(r, 0.001f, std::numeric_limits<float>::max(), rec);
	}

private:
	std::vector<Object*> objects; // 所有的原始物体
	Object* bvh_root = nullptr;   // 加速结构根节点
	bool dirty = false;
};