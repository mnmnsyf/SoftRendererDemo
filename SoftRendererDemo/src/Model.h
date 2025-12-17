#pragma once
#include <string>
#include <vector>
#include "Geometry.h" // 包含 Mesh, Vec3f, Vec2f

class Model {
public:
	// 构造函数直接加载
	Model(const std::string& filepath);
	~Model() = default;

	// 获取生成的 Mesh 数据
	Mesh get_mesh() const { return _mesh; }

private:
	Mesh _mesh;

	// 内部辅助结构：用于暂存 OBJ 文件中的原始数据
	// 因为 OBJ 的 f 指令引用的索引是基于这些数组的
	std::vector<Vec3f> _raw_positions;
	std::vector<Vec3f> _raw_normals;
	std::vector<Vec2f> _raw_uvs;

	// 解析具体的一行
	void load_obj(const std::string& filepath);

	// 辅助：解析 "v/vt/vn" 这种字符串，返回三个索引
	// 返回值：{pos_idx, uv_idx, norm_idx}，如果没有则为 -1
	struct ObjIndex { int p = -1, t = -1, n = -1; };
	ObjIndex parse_face_index(const std::string& token);
};