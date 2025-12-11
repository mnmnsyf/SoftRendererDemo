#include <iostream>
#include "Math.h"
#include "Rasterizer.h" // Include Rasterizer

// Demo: Draw a rainbow triangle
void run_rainbow_triangle_demo() {
	// 1. Initialize Rasterizer (800x600)
	const int width = 800;
	const int height = 600;
	Rasterizer r(width, height);

	// 2. Prepare MVP Matrices
	Vec3f eye(0, 0, 0);
	Vec3f center(0, 0, -1);
	Vec3f up(0, 1, 0);
	Mat4 view = Mat4::lookAt(eye, center, up);
	Mat4 proj = Mat4::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
	Mat4 viewport = Mat4::viewport(width, height);

	// 3. Define a triangle (at Z = -5)
	Vec4f v1(0.0f, 0.5f, -2.0f, 1.0f); // Top
	Vec4f v2(-0.5f, -0.5f, -2.0f, 1.0f); // Bottom Left
	Vec4f v3(0.5f, -0.5f, -2.0f, 1.0f); // Bottom Right
	Vec4f raw_verts[] = { v1, v2, v3 };

	// Define vertex colors (RGB)
	Vec3f c1(1.0f, 0.0f, 0.0f); // Red
	Vec3f c2(0.0f, 1.0f, 0.0f); // Green
	Vec3f c3(0.0f, 0.0f, 1.0f); // Blue
	Vec3f colors[] = { c1, c2, c3 };

	// Store transformed screen coordinates
	Vec3f screen_verts[3];

	// 4. Vertex Shader Pipeline
	for (int i = 0; i < 3; i++) {
		// MVP Transform -> Clip Space
		Vec4f clip = proj * view * raw_verts[i];

		// Perspective Division -> NDC
		// Note: dividing by w is critical for perspective
		Vec3f ndc;
		ndc.x = clip.x / clip.w;
		ndc.y = clip.y / clip.w;
		ndc.z = clip.z / clip.w;

		// Viewport Transform -> Screen Space
		Vec4f screen = viewport * Vec4f(ndc.x, ndc.y, ndc.z, 1.0f);

		// Store in array (take x, y, z)
		screen_verts[i] = Vec3f(screen.x, screen.y, screen.z);
	}

	// 5. Rasterization
	// Pass screen coordinates and colors
	r.clear(Vec3f(0.1f, 0.1f, 0.1f)); // Background: Dark Gray
	r.draw_triangle(screen_verts[0], screen_verts[1], screen_verts[2], colors);

	// 6. Save Result
	r.save_to_ppm("rainbow_triangle.ppm");
}

// Helper: Define a test scene
// order_flag: true (Red first), false (Blue first)
void run_z_buffer_test(Rasterizer& r, bool draw_red_first) {
	// 1. Clear Screen (Color & Depth Buffer)
	r.clear(Vec3f(0, 0, 0));

	// ==========================================
	// Define triangle data (Screen Space + Z)
	// Screen size 800x600
	// ==========================================

	// Triangle 1: Red, Z = 0.5 (Near)
	// Upright triangle
	Vec3f t1_v0(400, 500, 0.5f); // Top
	Vec3f t1_v1(200, 150, 0.2f); // Bottom Left
	Vec3f t1_v2(600, 150, 0.1f); // Bottom Right
	Vec3f t1_color[3] = { {1, 0, 0}, {1, 0, 0}, {1, 0, 0} }; // Red

	// Triangle 2: Blue, Z = 0.9 (Far)
	// Inverted triangle
	Vec3f t2_v0(400, 100, 0.5f); // Bottom
	Vec3f t2_v1(200, 450, 0.7f); // Top Left
	Vec3f t2_v2(600, 450, 0.9f); // Top Right
	Vec3f t2_color[3] = { {0, 0, 1}, {0, 0, 1}, {0, 0, 1} }; // Blue

	// ==========================================
	// Draw
	// ==========================================
	if (draw_red_first) {
		std::cout << "Test Mode: Draw Red (Near) first, then Blue (Far)..." << std::endl;

		// 1. Draw Red (Z=0.5)
		// Depth Buffer writes 0.5
		r.draw_triangle(t1_v0, t1_v1, t1_v2, t1_color);

		// 2. Draw Blue (Z=0.9)
		// Z-Test: 0.9 < 0.5 ? False -> Discard Blue
		r.draw_triangle(t2_v0, t2_v1, t2_v2, t2_color);

		r.save_to_ppm("z_test_red_first.ppm");
		// Export Depth Map
		r.save_depth_to_ppm("depth_map.ppm");
	}
	else {
		std::cout << "Test Mode: Draw Blue (Far) first, then Red (Near)..." << std::endl;

		// 1. Draw Blue (Z=0.9)
		// Depth Buffer writes 0.9
		r.draw_triangle(t2_v0, t2_v1, t2_v2, t2_color);

		// 2. Draw Red (Z=0.5)
		// Z-Test: 0.5 < 0.9 ? True -> Red covers Blue
		r.draw_triangle(t1_v0, t1_v1, t1_v2, t1_color);

		r.save_to_ppm("z_test_blue_first.ppm");
	}
}

int main() {

	Rasterizer r(800, 600);

	// Test 1: Far(Blue) then Near(Red)
	// "Painter's Algorithm" order (Reference)
	run_z_buffer_test(r, false);

	// Test 2: Near(Red) then Far(Blue)
	// Critical for Z-Buffer!
	// Without Z-Buffer: Blue covers Red.
	// With Z-Buffer: Blue is occluded.
	run_z_buffer_test(r, true);

	std::cout << "Test Finished! Please check the generated .ppm files." << std::endl;

    return 0;
}
