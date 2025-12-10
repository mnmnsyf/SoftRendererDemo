#include "Rasterizer.h"
#include <algorithm> // 用于 std::min, std::max
#include <fstream>   // 用于文件写入
#include <iostream>
#include <tuple> // 用于返回多个值

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
    // 初始化 FrameBuffer，大小为 w * h，默认全黑
    frame_buffer.resize(w * h, Vec3f(0, 0, 0));
}

void Rasterizer::clear(const Vec3f& color) {
    std::fill(frame_buffer.begin(), frame_buffer.end(), color);
}

void Rasterizer::set_pixel(int x, int y, const Vec3f& color) {
    // 1. 边界检查 (这一步非常重要！)
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    // 2. 计算一维索引
    // 我们习惯 (0,0) 在左下角，但存储通常是从左上开始或者线性存储
    // 这里我们定义：index = y * width + x
    // 这样 y=0 是第一行（最下面），y=height-1 是最上面
    int index = y * width + x;
    frame_buffer[index] = color;
}

// =============================================
// 核心算法：判断点是否在三角形内 (利用叉乘)
// =============================================
// 原理：如果点 P 在三角形 ABC 内部，那么：
// P 在 AB 的左边，P 在 BC 的左边，P 在 CA 的左边 (假设逆时针)
// 数学上体现为：叉乘结果 Z 值同号
static float cross_product_2d(float x1, float y1, float x2, float y2) {
    return x1 * y2 - x2 * y1;
}

bool Rasterizer::inside(float x, float y, const Vec3f* _v) {
    // 准备三个顶点的坐标
    const Vec3f& A = _v[0];
    const Vec3f& B = _v[1];
    const Vec3f& C = _v[2];

    // 准备点 P 的坐标
    // 注意：我们只关心 xy 平面

    // 计算三个叉乘
    // 1. 边 AB 和 向量 AP
    float cp1 = cross_product_2d(B.x - A.x, B.y - A.y, x - A.x, y - A.y);
    // 2. 边 BC 和 向量 BP
    float cp2 = cross_product_2d(C.x - B.x, C.y - B.y, x - B.x, y - B.y);
    // 3. 边 CA 和 向量 CP
    float cp3 = cross_product_2d(A.x - C.x, A.y - C.y, x - C.x, y - C.y);

    // 如果三个叉乘结果符号相同（都在左侧或都在右侧），则在内部
    return (cp1 >= 0 && cp2 >= 0 && cp3 >= 0) || (cp1 <= 0 && cp2 <= 0 && cp3 <= 0);
}

// =============================================
// 辅助：计算重心坐标
// 输入：三角形三个点 A, B, C 和 当前像素坐标 P(x, y)
// 输出：tuple(alpha, beta, gamma)
// =============================================
static std::tuple<float, float, float> compute_barycentric_2d(float x, float y, const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
    // ---------------------------------------------------------
    // 使用标准的代数公式计算重心坐标
    // 这种写法比直接用叉乘更不容易出错，因为它明确了每一项的代数符号
    // ---------------------------------------------------------

    // 1. 计算分母 (总面积的 2 倍)
    // 公式: (y1 - y2) * x0 + (x2 - x1) * y0 + x1 * y2 - x2 * y1
    float c1 = (v1.y - v2.y) * v0.x + (v2.x - v1.x) * v0.y + v1.x * v2.y - v2.x * v1.y;

    // 如果面积接近0，直接视为无效
    if (std::abs(c1) < 1e-5) return { -1, 1, 1 };

    // 2. 计算 Alpha (对应 v0 的权重)
    // 使用点 P(x,y) 替换公式中的 v0
    float c2 = (v1.y - v2.y) * x + (v2.x - v1.x) * y + v1.x * v2.y - v2.x * v1.y;
    float alpha = c2 / c1;

    // 3. 计算 Beta (对应 v1 的权重)
    // 使用点 P(x,y) 替换公式中的 v1 (注意公式结构变化: v0, P, v2)
    // 公式: (y2 - y0) * x + (x0 - x2) * y + x2 * y0 - x0 * y2
    float c3 = (v2.y - v0.y) * x + (v0.x - v2.x) * y + v2.x * v0.y - v0.x * v2.y;
    float beta = c3 / c1;

    // 4. Gamma 自动补齐
    float gamma = 1.0f - alpha - beta;

    return { alpha, beta, gamma };
}

// =============================================
// 核心流程：光栅化三角形
// =============================================
void Rasterizer::draw_triangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Vec3f* colors) {
    // 1. 包围盒计算 (保持不变)
    float min_x = std::min({ v0.x, v1.x, v2.x });
    float max_x = std::max({ v0.x, v1.x, v2.x });
    float min_y = std::min({ v0.y, v1.y, v2.y });
    float max_y = std::max({ v0.y, v1.y, v2.y });

    int x_start = std::max(0, (int)floor(min_x));
    int x_end = std::min(width - 1, (int)ceil(max_x));
    int y_start = std::max(0, (int)floor(min_y));
    int y_end = std::min(height - 1, (int)ceil(max_y));

    // 2. 遍历像素
    for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
            // 采样中心点
            float px = x + 0.5f;
            float py = y + 0.5f;

            // 3. 计算重心坐标
            auto [alpha, beta, gamma] = compute_barycentric_2d(px, py, v0, v1, v2);

            // 4. 判断是否在三角形内
            // 只要有一个分量小于 0，说明在三角形外面
            if (alpha >= 0 && beta >= 0 && gamma >= 0) {

                // 5. 【核心逻辑】颜色插值
                // Color = c0 * alpha + c1 * beta + c2 * gamma
                Vec3f interpolated_color =
                    colors[0] * alpha +
                    colors[1] * beta +
                    colors[2] * gamma;

                // 写入 Framebuffer
                set_pixel(x, y, interpolated_color);
            }
        }
    }
}

// =============================================
// 导出图片 PPM 格式
// =============================================
void Rasterizer::save_to_ppm(const char* filename) {
    std::ofstream ofs(filename);

    // 写入 PPM 头部
    // P3 表示 ASCII 格式，接着是 宽 高，最后是最大颜色值 255
    ofs << "P3\n" << width << " " << height << "\n255\n";

    // 写入像素数据
    // 注意：PPM 格式通常是从左上角开始记录，而我们的 buffer 是从左下角存的 (y=0)
    // 所以我们倒着遍历 Y 轴
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            Vec3f color = frame_buffer[index];

            // 将 0.0~1.0 的浮点颜色转换为 0~255 的整数
            // 使用 clamp 防止溢出
            int r = std::min(255, std::max(0, (int)(color.x * 255.0f)));
            int g = std::min(255, std::max(0, (int)(color.y * 255.0f)));
            int b = std::min(255, std::max(0, (int)(color.z * 255.0f)));

            ofs << r << " " << g << " " << b << " ";
        }
        ofs << "\n";
    }

    ofs.close();
    std::cout << "Image saved to " << filename << std::endl;
}