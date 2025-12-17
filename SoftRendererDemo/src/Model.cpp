#include "Model.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Model::Model(const std::string& filepath) {
	load_obj(filepath);
}

// 辅助函数：简单的字符串分割
std::vector<std::string> split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

void Model::load_obj(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) {
		std::cerr << "Error: Failed to load model " << filepath << std::endl;
		return;
	}

	_raw_positions.reserve(10000);
	_raw_normals.reserve(10000);
	_raw_uvs.reserve(10000);
	_mesh.positions.reserve(_raw_positions.size() * 2);
	_mesh.normals.reserve(_raw_positions.size() * 2);
	_mesh.uvs.reserve(_raw_positions.size() * 2);
	_mesh.indices.reserve(_raw_positions.size() * 6);

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;

		std::stringstream ss(line);
		std::string prefix;
		ss >> prefix;

		// 1. 解析顶点位置 v x y z
		if (prefix == "v") {
			Vec3f p;
			ss >> p.x >> p.y >> p.z;
			_raw_positions.push_back(p);
		}
		// 2. 解析纹理坐标 vt u v
		else if (prefix == "vt") {
			Vec2f t;
			ss >> t.x >> t.y;
			// 这是一个坑：很多 OBJ 的 V 坐标和 OpenGL/DirectX 是垂直翻转的
			// 这里我们暂时保持原样，如果渲染出来贴图倒了，改成 t.y = 1.0f - t.y;
			_raw_uvs.push_back(t);
		}
		// 3. 解析法线 vn x y z
		else if (prefix == "vn") {
			Vec3f n;
			ss >> n.x >> n.y >> n.z;
			_raw_normals.push_back(n);
		}
		// 4. 解析面 f v1/vt1/vn1 v2/vt2/vn2 ...
		else if (prefix == "f") {
			std::vector<ObjIndex> face_indices;
			std::string token;

			// 读取该面包含的所有顶点索引块
			while (ss >> token) {
				face_indices.push_back(parse_face_index(token));
			}

			// 三角化 (Triangulation)
			// OBJ 可能包含四边形(Quad)或多边形，我们需要把它们拆成三角形
			// 扇形拆分法：(0, 1, 2), (0, 2, 3), (0, 3, 4)...
			for (size_t i = 1; i < face_indices.size() - 1; ++i) {
				ObjIndex idx0 = face_indices[0];
				ObjIndex idx1 = face_indices[i];
				ObjIndex idx2 = face_indices[i + 1];

				// 构建三个顶点
				ObjIndex tri_indices[3] = { idx0, idx1, idx2 };

				for (int k = 0; k < 3; ++k) {
					ObjIndex idx = tri_indices[k];

					// --- 处理 Position ---
					// OBJ 索引从 1 开始，记得 -1
					if (idx.p >= 0 && idx.p < _raw_positions.size()) {
						_mesh.positions.push_back(_raw_positions[idx.p]);
					}
					else {
						_mesh.positions.push_back(Vec3f(0, 0, 0)); // 容错
					}

					// --- 处理 UV ---
					if (idx.t >= 0 && idx.t < _raw_uvs.size()) {
						_mesh.uvs.push_back(_raw_uvs[idx.t]);
					}
					else {
						_mesh.uvs.push_back(Vec2f(0, 0)); // 默认 UV
					}

					// --- 处理 Normal ---
					if (idx.n >= 0 && idx.n < _raw_normals.size()) {
						_mesh.normals.push_back(_raw_normals[idx.n]);
					}
					else {
						// 如果没有法线，暂时填个向上的，或者之后重新计算
						_mesh.normals.push_back(Vec3f(0, 1, 0));
					}

					// --- 处理 Indices ---
					// 因为我们是展平存储 (Flatten)，所以直接线性增加索引即可
					_mesh.indices.push_back((int)_mesh.positions.size() - 1);
				}
			}
		}
	}

	std::cout << "Model loaded: " << filepath
		<< " (" << _mesh.indices.size() / 3 << " tris)" << std::endl;
}

// 解析 "1/2/3" 或 "1//3" 或 "1"
Model::ObjIndex Model::parse_face_index(const std::string& token) {
	ObjIndex result;

	// 查找第一个斜杠
	size_t first_slash = token.find('/');

	// 情况 1: 只有位置索引 "v"
	if (first_slash == std::string::npos) {
		result.p = std::stoi(token) - 1;
		return result;
	}

	// 解析位置索引 (第一个斜杠前)
	std::string p_str = token.substr(0, first_slash);
	if (!p_str.empty()) result.p = std::stoi(p_str) - 1;

	// 查找第二个斜杠
	size_t second_slash = token.find('/', first_slash + 1);

	// 情况 2: "v/vt" (没有法线)
	if (second_slash == std::string::npos) {
		std::string t_str = token.substr(first_slash + 1);
		if (!t_str.empty()) result.t = std::stoi(t_str) - 1;
		return result;
	}

	// 情况 3: "v/vt/vn" 或 "v//vn"
	// 解析纹理索引 (两个斜杠中间)
	std::string t_str = token.substr(first_slash + 1, second_slash - first_slash - 1);
	if (!t_str.empty()) result.t = std::stoi(t_str) - 1;

	// 解析法线索引 (第二个斜杠后)
	std::string n_str = token.substr(second_slash + 1);
	if (!n_str.empty()) result.n = std::stoi(n_str) - 1;

	return result;
}