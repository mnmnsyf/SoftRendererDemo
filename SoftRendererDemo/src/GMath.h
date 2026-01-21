#pragma once
#include <cmath>
#include <random> 

struct Vec2f {
	float x, y;

	Vec2f() : x(0.0f), y(0.0f) {}
    Vec2f(float _x, float _y) : x(_x), y(_y) {}

	// 运算符重载
	Vec2f operator+(const Vec2f& rhs) const;
	Vec2f operator*(float scalar) const;
	Vec2f operator/(float scalar) const;
};

struct Vec3f {
    float x, y, z;
    // 1. 构造函数
    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float scalar) : x(scalar), y(scalar), z(scalar) {}
    Vec3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // 2. 基础运算
    Vec3f operator+(float scalar) const; // 加法 (向量 + 标量)
    Vec3f operator+(const Vec3f& rhs) const; // 加法
    Vec3f operator-(const Vec3f& rhs) const; // 减法
    Vec3f operator*(float scalar) const;    // 数乘 (向量 * 标量)
    Vec3f operator/(float scalar) const;    // 除法
    Vec3f operator*(const Vec3f& v) const;  // 向量与向量的分量相乘 (Hadamard Product)

    float operator[](int i) const;          // 访问x, y, z 成员变量
    float& operator[](int i);               

    // 3. 高级运算
    float dot(const Vec3f& rhs) const;       // 点乘
    Vec3f cross(const Vec3f& rhs) const;     // 叉乘
    Vec3f normalize() const;                 // 归一化
    float length() const;                    // 长度
};

struct Vec4f {
    float x, y, z, w;

    Vec4f() : x(0), y(0), z(0), w(0) {}
    Vec4f(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

    // 支持从 Vec3f + w 构造
    Vec4f(const Vec3f& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

	Vec4f operator+(const Vec4f& rhs) const; // 向量加法
	Vec4f operator-(const Vec4f& rhs) const; // 向量减法
	Vec4f operator*(float scalar) const;     // 数乘
	Vec4f operator/(float scalar) const;     // 除法

    Vec3f xyz() const { return Vec3f(x, y, z); }
};

struct Mat4 {
    float m[4][4]; // 或 float m[16];

    Mat4();
    static Mat4 identity();
    Mat4 operator*(const Mat4& rhs) const;
    Vec4f operator*(const Vec4f& v) const;

    // 平移矩阵 (x, y, z)
    static Mat4 translate(float x, float y, float z);
    static Mat4 translate(const Vec3f& v); // 重载版本，方便直接传向量

    // 缩放矩阵 (x, y, z)
    static Mat4 scale(float x, float y, float z);
    static Mat4 scale(const Vec3f& v);

    // 旋转矩阵 (输入角度，单位：度 Degree)
    static Mat4 rotateX(float angleDeg);
    static Mat4 rotateY(float angleDeg);
    static Mat4 rotateZ(float angleDeg);

    // 摄像机：位置(eye)，看向哪里(center)，头顶朝向(up)
    static Mat4 lookAt(const Vec3f& eye, const Vec3f& center, const Vec3f& up);

    // 透视投影：垂直视角(fovY, 角度)，宽高比(aspect)，近平面(zNear)，远平面(zFar)
    static Mat4 perspective(float fovY, float aspect, float zNear, float zFar);

    // 视口变换：输入屏幕宽高
    static Mat4 viewport(float width, float height);
};

struct GMath {
	// 标量 lerp
	static float lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	// 向量 lerp
	static Vec3f lerp(const Vec3f& a, const Vec3f& b, float t) {
		return a + (b - a) * t;
	}
};

