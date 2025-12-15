#pragma once
#include <vector>
#include "Math.h"

// 基础网格结构
struct Mesh {
	std::vector<Vec3f> positions;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> uvs;
	std::vector<int> indices;
};

// 几何体生成工厂函数
Mesh generate_sphere(float radius, int slices, int stacks, bool use_flat_normals);
Mesh generate_sphere(float radius, int slices, int stacks);
Mesh generate_plane(float size, float z_depth, float uv_scale);
Mesh generate_quad();