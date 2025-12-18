#include "Geometry.h"
#include <cmath>

// ========================================================================
// generate_mesh
// ========================================================================
namespace Geometry {
	Mesh generate_sphere(float radius, int slices, int stacks) {
		Mesh mesh;
		float pi = 3.14159265359f;
		for (int j = 0; j <= stacks; ++j) {
			float v = (float)j / stacks;
			float phi = v * pi;
			for (int i = 0; i <= slices; ++i) {
				float u = (float)i / slices;
				float theta = u * 2.0f * pi;
				float x = radius * std::sin(phi) * std::cos(theta);
				float y = radius * std::cos(phi);
				float z = radius * std::sin(phi) * std::sin(theta);
				mesh.positions.push_back(Vec3f(x, y, z));
				mesh.normals.push_back(Vec3f(x, y, z).normalize());
				mesh.uvs.push_back(Vec2f(u, v));
			}
		}
		for (int j = 0; j < stacks; ++j) {
			for (int i = 0; i < slices; ++i) {
				int p0 = j * (slices + 1) + i;
				int p1 = p0 + 1;
				int p2 = (j + 1) * (slices + 1) + i;
				int p3 = p2 + 1;
				mesh.indices.push_back(p0); mesh.indices.push_back(p2); mesh.indices.push_back(p1);
				mesh.indices.push_back(p1); mesh.indices.push_back(p2); mesh.indices.push_back(p3);
			}
		}
		return mesh;
	}

	Mesh generate_plane(float size, float z_depth, float uv_scale) {
		Mesh mesh;
		mesh.positions = { {-size, -1.0f, 0.0f}, {size, -1.0f, 0.0f}, {-size, -1.0f, z_depth}, {size, -1.0f, z_depth} };
		mesh.normals = { {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0} };
		mesh.uvs = { {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, uv_scale}, {1.0f, uv_scale} };
		mesh.indices = { 0, 1, 2, 1, 3, 2 };
		return mesh;
	}

	Mesh generate_quad() {
		Mesh mesh;
		mesh.positions = { {-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0} };
		mesh.normals = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1} };
		mesh.uvs = { {0, 0}, {1, 0}, {0, 1}, {1, 1} };
		mesh.indices = { 0, 1, 2, 1, 3, 2 };
		return mesh;
	}

	// ==========================================
	// 辅助：生成球体数据
	// radius: 半径
	// slices, stacks: 细分程度 (比如 20, 20)
	// use_flat_normals: 
	//    true  -> 生成“面法线” (Flat Shading 效果，顶点不共用，法线垂直于面)
	//    false -> 生成“顶点法线” (Smooth/Phong 效果，顶点共用，法线指向外)
	// ==========================================
	Mesh generate_sphere(float radius, int slices, int stacks, bool use_flat_normals) {
		Mesh mesh;
		float pi = 3.14159265359f;

		if (use_flat_normals) {
			// --- 模式 A: Flat Shading (独立三角形，硬棱角) ---
			// 这种模式下，我们直接生成互不相连的三角形
			for (int i = 0; i < slices; ++i) {
				for (int j = 0; j < stacks; ++j) {
					// 计算一个 Quad 的 4 个角点 (theta: 经度, phi: 纬度)
					// 确定球面上任何一个点，只需要知道三个数：半径(Radius, r)：球有多大。
					// 经度(Longitude, theta)：水平方向的角度，转一圈是 0 到 2pi(360度)。
					// 纬度(Latitude, phi)：垂直方向的角度，从北极到南极是 0 到 pi(180度)。
					float theta1 = (float)i / slices * 2 * pi;
					float theta2 = (float)(i + 1) / slices * 2 * pi;
					float phi1 = (float)j / stacks * pi;
					float phi2 = (float)(j + 1) / stacks * pi;

					// 简单的球面坐标转笛卡尔坐标 lambda
					auto get_pos = [&](float theta, float phi) {
						return Vec3f(
							radius * sin(phi) * cos(theta),
							radius * cos(phi), // Y-up
							radius * sin(phi) * sin(theta)
						);
						};

					Vec3f p0 = get_pos(theta1, phi1);// 当前点
					Vec3f p1 = get_pos(theta2, phi1);// 右边的点
					Vec3f p2 = get_pos(theta1, phi2);// 下边的点
					Vec3f p3 = get_pos(theta2, phi2);// 右下角的点

					// 拆分成两个三角形: T1(p0, p2, p1), T2(p1, p2, p3)
					// 计算面法线 (Face Normal)
					Vec3f n1 = (p1 - p0).cross(p2 - p0).normalize();
					Vec3f n2 = (p3 - p1).cross(p2 - p1).normalize();

					// 存入 T1
					mesh.positions.push_back(p0); mesh.normals.push_back(n1);
					mesh.positions.push_back(p2); mesh.normals.push_back(n1);
					mesh.positions.push_back(p1); mesh.normals.push_back(n1);

					// 存入 T2
					mesh.positions.push_back(p1); mesh.normals.push_back(n2);
					mesh.positions.push_back(p2); mesh.normals.push_back(n2);
					mesh.positions.push_back(p3); mesh.normals.push_back(n2);
				}
			}
		}
		else {
			// --- 模式 B: Smooth Shading (共享顶点，插值法线) ---
			// 这种模式下，法线就是 Vertex -> Pos 的方向
			for (int j = 0; j <= stacks; ++j) {
				float phi = (float)j / stacks * pi;
				for (int i = 0; i <= slices; ++i) {
					float theta = (float)i / slices * 2 * pi;

					float x = sin(phi) * cos(theta);
					float y = cos(phi);
					float z = sin(phi) * sin(theta);

					mesh.positions.push_back(Vec3f(x * radius, y * radius, z * radius));
					mesh.normals.push_back(Vec3f(x, y, z)); // 归一化的法线
				}
			}

			// 生成索引 (EBO) - 这里我们为了 Rasterizer 接口简单，手动展开成纯顶点数组
			// 如果你的 Rasterizer 支持 draw_elements 可以直接用 indices
			// 这里做一个临时的转换：Mesh 结构虽然有 indices 但我们在下面手动展平它
			std::vector<Vec3f> flat_pos;
			std::vector<Vec3f> flat_nor;

			for (int j = 0; j < stacks; ++j) {
				for (int i = 0; i < slices; ++i) {
					int p0 = j * (slices + 1) + i;
					int p1 = p0 + 1;
					int p2 = (j + 1) * (slices + 1) + i;
					int p3 = p2 + 1;

					// T1: p0, p2, p1
					flat_pos.push_back(mesh.positions[p0]); flat_nor.push_back(mesh.normals[p0]);
					flat_pos.push_back(mesh.positions[p2]); flat_nor.push_back(mesh.normals[p2]);
					flat_pos.push_back(mesh.positions[p1]); flat_nor.push_back(mesh.normals[p1]);

					// T2: p1, p2, p3
					flat_pos.push_back(mesh.positions[p1]); flat_nor.push_back(mesh.normals[p1]);
					flat_pos.push_back(mesh.positions[p2]); flat_nor.push_back(mesh.normals[p2]);
					flat_pos.push_back(mesh.positions[p3]); flat_nor.push_back(mesh.normals[p3]);
				}
			}
			mesh.positions = flat_pos;
			mesh.normals = flat_nor;
		}
		return mesh;
	}
}

// ==========================================
// Bezier 实现
// ==========================================
namespace Geometry {
	// 核心求值函数 (递归版 De Casteljau)
	// 输入: 当前层级的控制点列表, 参数 t
	// 输出: 最终计算出的点
	Vec3f Bezier::eval(const std::vector<Vec3f>& pts, float t) {
		if (pts.empty()) return Vec3f(0, 0, 0);

		// 拷贝一份控制点作为工作副本
		std::vector<Vec3f> temp = pts;
		int n = temp.size();

		// De Casteljau 迭代过程：类似金字塔倒推
		// k 表示层级，从 n-1 降到 1
		for (int k = 1; k < n; ++k) {
			for (int i = 0; i < n - k; ++i) {
				// 原地更新数组：P_i = (1-t)P_i + t*P_{i+1}
				temp[i] = temp[i] + (temp[i + 1] - temp[i]) * t;
			}
		}

		// 最终结果汇聚在第一个元素
		return temp[0];
	}

	// 生成整条曲线的所有点
	// segments: 将 t=0 到 t=1 分割成多少段
	std::vector<Vec3f> Bezier::generate_curve(const std::vector<Vec3f>& control_points, int segments) {
		std::vector<Vec3f> curve_points;
		curve_points.reserve(segments + 1);

		for (int i = 0; i <= segments; ++i) {
			float t = (float)i / (float)segments;

			// 对每个 t 计算坐标
			Vec3f p = eval(control_points, t);

			curve_points.push_back(p);
		}

		return curve_points;
	}

	Vec3f Bezier::eval_surface(const std::vector<Vec3f>& control_points, float u, float v)
	{
		// 安全检查：必须是 16 个点 (4x4)
		if (control_points.size() != 16) {
			return Vec3f(0, 0, 0);
		}

		// 临时容器：存储 4 行经过 u 插值后的点
		std::vector<Vec3f> u_interpolated_points;
		u_interpolated_points.reserve(4);

		// 1. 遍历 4 行
		for (int i = 0; i < 4; ++i) {
			// 提取当前行的 4 个控制点
			std::vector<Vec3f> row_points;
			row_points.reserve(4);

			for (int j = 0; j < 4; ++j) {
				// 索引计算：行号 * 4 + 列号
				row_points.push_back(control_points[i * 4 + j]);
			}

			// 对这一行用 u 进行曲线求值，得到一个点
			Vec3f p = eval(row_points, u);
			u_interpolated_points.push_back(p);
		}

		// 2. 对生成的 4 个点，用 v 进行曲线求值
		// 这时 u_interpolated_points 变成了竖向的一条贝塞尔曲线的控制点
		return eval(u_interpolated_points, v);
	}

	Mesh Bezier::generate_surface_mesh(const std::vector<Vec3f>& control_points, int div_u, int div_v) {
		Mesh mesh;
		// 1. 生成顶点数据 (Positions & Normals & UVs)
		// 注意：顶点数比格子数多 1
		for (int i = 0; i <= div_v; ++i) {
			float v = (float)i / div_v;
			for (int j = 0; j <= div_u; ++j) {
				float u = (float)j / div_u;

				// 计算位置
				Vec3f pos = eval_surface(control_points, u, v);
				mesh.positions.push_back(pos);

				// 计算 UV
				mesh.uvs.push_back(Vec2f(u, v));

				// 计算法线 (使用有限差分法，防止边缘导数退化)
				float eps = 0.001f;
				// 限制采样点在 [0,1] 范围内
				float u_L = std::max(0.0f, u - eps);
				float u_R = std::min(1.0f, u + eps);
				float v_B = std::max(0.0f, v - eps);
				float v_T = std::min(1.0f, v + eps);

				Vec3f p_L = eval_surface(control_points, u_L, v);
				Vec3f p_R = eval_surface(control_points, u_R, v);
				Vec3f p_B = eval_surface(control_points, u, v_B);
				Vec3f p_T = eval_surface(control_points, u, v_T);

				Vec3f tangent_u = (p_R - p_L).normalize();
				Vec3f tangent_v = (p_T - p_B).normalize();

				// 叉乘得到法线
				mesh.normals.push_back(tangent_u.cross(tangent_v).normalize());
			}
		}

		// 2. 生成三角形索引 (Indices)
		// 注意：循环只到 div_v - 1 和 div_u - 1，防止越界
		int n_verts_row = div_u + 1;
		for (int i = 0; i < div_v; ++i) {
			for (int j = 0; j < div_u; ++j) {
				int p0 = i * n_verts_row + j;       // 左下
				int p1 = p0 + 1;                    // 右下
				int p2 = (i + 1) * n_verts_row + j; // 左上
				int p3 = p2 + 1;                    // 右上

				// 三角形 1: p0 -> p1 -> p2
				mesh.indices.push_back(p0);
				mesh.indices.push_back(p1);
				mesh.indices.push_back(p2);

				// 三角形 2: p1 -> p3 -> p2
				mesh.indices.push_back(p1);
				mesh.indices.push_back(p3);
				mesh.indices.push_back(p2);
			}
		}

		return mesh;
	}
}