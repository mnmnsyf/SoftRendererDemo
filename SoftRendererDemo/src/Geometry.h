#pragma once
#include <vector>
#include "GMath.h"

// 基础网格结构
struct Mesh {
	std::vector<Vec3f> positions;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> uvs;
	std::vector<int> indices;
};

// 几何体生成工厂函数
namespace Geometry {
	// --- 3D 几何体生成 ---
	Mesh generate_sphere(float radius, int slices, int stacks, bool use_flat_normals);
	Mesh generate_sphere(float radius, int slices, int stacks);
	Mesh generate_plane(float size, float z_depth, float uv_scale);
	Mesh generate_quad();

	// 贝塞尔曲线
	struct Bezier {
		// 核心求值 (De Casteljau)
		static Vec3f eval(const std::vector<Vec3f>& pts, float t);

		// 生成整条线的点集
		static std::vector<Vec3f> generate_curve(const std::vector<Vec3f>& pts, int segments);

		// 曲面求值 双向 De Casteljau
		// control_points: 必须包含 16 个点 (4x4 矩阵的行优先排列)
		static Vec3f eval_surface(const std::vector<Vec3f>& control_points, float u, float v);

		// 将曲面细分为三角形网格
		// control_points: 16个点
		// div_u、div_v: 分段数量 (比如 10，则生成 10x10=100 个方格)
		static Mesh generate_surface_mesh(const std::vector<Vec3f>& control_points, int div_u, int div_v);
	};
};