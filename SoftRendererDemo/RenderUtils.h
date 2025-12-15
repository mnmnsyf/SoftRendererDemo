#pragma once
#include "Geometry.h"
#include "Shader.h"

// 将 Mesh 数据绑定到 Shader 的 Attribute
void bind_mesh_to_shader(const Mesh& mesh, BlinnPhongShader& shader);

// 初始化 Shader 的通用光照和矩阵参数
void setup_base_shader(BlinnPhongShader& shader, int w, int h);