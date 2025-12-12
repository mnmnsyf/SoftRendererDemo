#include "Math.h"
#include <cmath>

// 辅助宏或常量：角度转弧度
// 公式：弧度 = 角度 * PI / 180.0
#define PI 3.14159265359f
#define TO_RAD(deg) ((deg) * PI / 180.0f)

// ==========================================
// Vec3f 实现
// ==========================================
Vec3f Vec3f::operator+(const Vec3f& rhs) const {
	return Vec3f(x + rhs.x, y + rhs.y, z + rhs.z);
}

Vec3f Vec3f::operator-(const Vec3f& rhs) const {
	return Vec3f(x - rhs.x, y - rhs.y, z - rhs.z);
}

Vec3f Vec3f::operator*(float scalar) const {
	return Vec3f(x * scalar, y * scalar, z * scalar);
}

Vec3f Vec3f::operator*(const Vec3f& v) const
{
    return Vec3f(x * v.x, y * v.y, z * v.z);
}

float Vec3f::dot(const Vec3f& rhs) const {
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

// 叉乘公式：
// x = y1*z2 - z1*y2
// y = z1*x2 - x1*z2
// z = x1*y2 - y1*x2
Vec3f Vec3f::cross(const Vec3f& rhs) const {
    return Vec3f(
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x
    );
}

float Vec3f::length() const {
	return std::sqrt(x * x + y * y + z * z);
}

Vec3f Vec3f::normalize() const {
    float len = length();
    if (len > 0.00001f) { // 防止除以0
        float invLen = 1.0f / len;
        return Vec3f(x * invLen, y * invLen, z * invLen);
    }
    return Vec3f(0, 0, 0);
}

// ==========================================
// Mat4 实现
// ==========================================

// 默认构造：清零
Mat4::Mat4() {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = 0.0f;
}

// 静态方法：生成单位矩阵
// 1 0 0 0
// 0 1 0 0
// 0 0 1 0
// 0 0 0 1
Mat4 Mat4::identity() {
    Mat4 res;
	for (int i = 0; i < 4; ++i)
        res.m[i][i] = 1.0f;
    return res;
}

// 矩阵 * 矩阵
// 公式：Result[i][j] = Row[i] · Col[j] (点积)
Mat4 Mat4::operator*(const Mat4& rhs) const {
    Mat4 res;
    for (int i = 0; i < 4; ++i) {     // 遍历行
        for (int j = 0; j < 4; ++j) { // 遍历列
            res.m[i][j] = 
                m[i][0] * rhs.m[0][j] +
                m[i][1] * rhs.m[1][j] +
                m[i][2] * rhs.m[2][j] +
                m[i][3] * rhs.m[3][j];
        }
    }
    return res;
}

// 矩阵 * 向量
// 结果向量的第 i 个分量 = 矩阵第 i 行与向量的点积
Vec4f Mat4::operator*(const Vec4f& v) const {
    return Vec4f(
        m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
        m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
    );
}

// 平移矩阵
// [ 1 0 0 tx ]
// [ 0 1 0 ty ]
// [ 0 0 1 tz ]
// [ 0 0 0 1  ]
Mat4 Mat4::translate(float x, float y, float z) {
    Mat4 res = identity(); // 先拿到单位矩阵
    res.m[0][3] = x;       // 第0行，第3列 (最后一列)
    res.m[1][3] = y;
    res.m[2][3] = z;
    return res;
}

Mat4 Mat4::translate(const Vec3f& v) {
    return translate(v.x, v.y, v.z);
}

// 缩放矩阵
// [ sx 0  0  0 ]
// [ 0  sy 0  0 ]
// [ 0  0  sz 0 ]
// [ 0  0  0  1 ]
Mat4 Mat4::scale(float x, float y, float z) {
    Mat4 res; // 默认全0
    res.m[0][0] = x;
    res.m[1][1] = y;
    res.m[2][2] = z;
    res.m[3][3] = 1.0f; // 别忘了 w 分量保持 1
    return res;
}

Mat4 Mat4::scale(const Vec3f& v) {
    return scale(v.x, v.y, v.z);
}

// 绕 X 轴旋转
// [ 1  0    0    0 ]
// [ 0 cos -sin   0 ]
// [ 0 sin  cos   0 ]
// [ 0  0    0    1 ]
Mat4 Mat4::rotateX(float angleDeg) {
    Mat4 res = identity();
    float rad = TO_RAD(angleDeg);
    float c = std::cos(rad);
    float s = std::sin(rad);

    res.m[1][1] = c;
    res.m[1][2] = -s;
    res.m[2][1] = s;
    res.m[2][2] = c;
    return res;
}

// 绕 Y 轴旋转
// [ cos  0  sin  0 ]
// [  0   1   0   0 ]
// [ -sin 0  cos  0 ]
// [  0   0   0   1 ]
Mat4 Mat4::rotateY(float angleDeg) {
    Mat4 res = identity();
    float rad = TO_RAD(angleDeg);
    float c = std::cos(rad);
    float s = std::sin(rad);

    res.m[0][0] = c;
    res.m[0][2] = s;
    res.m[2][0] = -s;
    res.m[2][2] = c;
    return res;
}

// 绕 Z 轴旋转
// [ cos -sin 0  0 ]
// [ sin  cos 0  0 ]
// [  0    0  1  0 ]
// [  0    0  0  1 ]
Mat4 Mat4::rotateZ(float angleDeg) {
    Mat4 res = identity();
    float rad = TO_RAD(angleDeg);
    float c = std::cos(rad);
    float s = std::sin(rad);

    res.m[0][0] = c;
    res.m[0][1] = -s;
    res.m[1][0] = s;
    res.m[1][1] = c;
    return res;
}

// ==========================================
// 视图矩阵 (LookAt)
// ==========================================
// 原理：构建一个新的坐标系 (Right, Up, Forward)，然后求逆
Mat4 Mat4::lookAt(const Vec3f& eye, const Vec3f& center, const Vec3f& up) {
    // 1. 计算视线方向 (Forward)
    // 注意：OpenGL 习惯中，Forward 是从 Target 指向 Eye (即 -Z 方向)
    Vec3f f = (eye - center);

    // 【防御 1】防止 eye 和 center 重合导致 NaN
    if (f.length() < 0.00001f) {
        // 视点和目标重合了，无法确定方向，直接返回单位矩阵或保持默认
        return Mat4::identity();
    }
    f = f.normalize();

    // 2. 计算右向量 (Right)
    Vec3f r = up.cross(f);

    // 【防御 2】防止视线和 Up 平行 (万向节死锁)
    // 如果 f 和 up 平行，cross 结果长度接近 0
    if (r.length() < 0.00001f) {
        // 补救措施：如果视线是垂直向上的，我们就假设“右”是 X 轴
        // 这种情况通常发生在你看向正上方或正下方时
        if (std::abs(f.y) > 0.999f) { // 正在看天或看地
            // 强制指定 Right 为 X 轴
            r = Vec3f(1.0f, 0.0f, 0.0f);
        }
        else {
            // 其他奇怪情况，随便找个轴（比如 Y 轴）叉乘一下
            r = Vec3f(0.0f, 1.0f, 0.0f).cross(f);
        }
    }
    r = r.normalize();

    // 3. 计算上向量 (True Up)
    // 既然 f 和 r 已经正交且归一化，u 自然也是归一化的
    Vec3f u = f.cross(r);

    // 4. 构建矩阵
    Mat4 res = identity();

    // 旋转部分 (转置的正交基)
    res.m[0][0] = r.x; res.m[0][1] = r.y; res.m[0][2] = r.z;
    res.m[1][0] = u.x; res.m[1][1] = u.y; res.m[1][2] = u.z;
    res.m[2][0] = f.x; res.m[2][1] = f.y; res.m[2][2] = f.z;

    // 平移部分
    res.m[0][3] = -r.dot(eye);
    res.m[1][3] = -u.dot(eye);
    res.m[2][3] = -f.dot(eye);

    return res;
}

// ==========================================
// 透视投影矩阵 (Perspective)
// ==========================================
Mat4 Mat4::perspective(float fovY, float aspect, float zNear, float zFar) {
    float tanHalfFovy = std::tan(TO_RAD(fovY) / 2.0f);

    Mat4 res; // 此时全为0

    // 1. 缩放 X 和 Y (实现视野范围控制)
    // 焦距 f = 1 / tan(fov/2)
    res.m[0][0] = 1.0f / (aspect * tanHalfFovy);
    res.m[1][1] = 1.0f / (tanHalfFovy);

    // 2. 深度处理 (Z mapping)
    // 将 zNear~zFar 映射到 -1~1 (或 0~1，取决于 API，这里按标准 GL -1~1)
    res.m[2][2] = -(zFar + zNear) / (zFar - zNear);

    // 3. 透视除法核心
    // 将 Z 值存入 W 分量。
    // 当 GPU 进行 (x/w, y/w, z/w) 时，实际上就是除以了 -z，从而实现近大远小
    res.m[3][2] = -1.0f;

    // 4. Z 的平移部分
    res.m[2][3] = -(2.0f * zFar * zNear) / (zFar - zNear);

    return res;
}

Mat4 Mat4::viewport(float width, float height) {
    Mat4 res = identity();

    // 1. X轴变换: [-1, 1] -> [0, width]
    res.m[0][0] = width / 2.0f;
    res.m[0][3] = width / 2.0f;

    // 2. Y轴变换: [-1, 1] -> [0, height]
    // 注意：这里假设屏幕原点在左下角（数学习惯）。
    // 如果你的屏幕原点在左上角（如 Windows 窗口、图片），需要把高度反转。公式变体：y_screen = (1 - y_ndc) * (h/2)
    res.m[1][1] = height / 2.0f;
    res.m[1][3] = height / 2.0f;

    // 3. Z轴变换: [-1, 1] -> [0, 1] (标准深度范围)
    res.m[2][2] = 0.5f;
    res.m[2][3] = 0.5f;

    return res;
}