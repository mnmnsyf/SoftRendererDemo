#pragma once
#include "Geometry.h"

class TestCC {
public:
	static void run_shading_test();
	static void run_specular_comparison();
	static void run_rainbow_triangle_demo();
	static void run_z_buffer_test();
	static void run_texture_test();
	static void run_integrated_test();
	static void scene_image_texture_test();
	static void normalize_mesh(Mesh& mesh);
	static void run_model_loading_test();
	static void run_turntable_animation();
};