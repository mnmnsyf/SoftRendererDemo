#include "RenderUtils.h"

void bind_mesh_to_shader(const Mesh& mesh, BlinnPhongShader& shader) {
	shader.in_positions.clear();
	shader.in_normals.clear();
	shader.in_uvs.clear();
	for (int idx : mesh.indices) {
		shader.in_positions.push_back(mesh.positions[idx]);
		shader.in_normals.push_back(mesh.normals[idx]);

		if (!mesh.uvs.empty()) shader.in_uvs.push_back(mesh.uvs[idx]);
		else shader.in_uvs.push_back(Vec2f(0, 0));
	}
}

void setup_base_shader(BlinnPhongShader& shader, int w, int h) {
	shader.projection = Mat4::perspective(45.0f, (float)w / h, 0.1f, 100.0f);
	shader.model = Mat4::identity();

	// 默认光照配置 (适配物理衰减的高强度)
	shader.light.position = Vec3f(5, 5, 5);
	shader.light.intensity = Vec3f(80, 80, 80);

	// 默认材质
	shader.k_a = Vec3f(0.1f, 0.1f, 0.1f);
	shader.k_d = Vec3f(0.8f, 0.8f, 0.8f);
	shader.k_s = Vec3f(1.0f, 1.0f, 1.0f);
	shader.p = 150.0f;
}