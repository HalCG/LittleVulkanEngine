#include "lve_camera.h"

// std
#include <cassert>
#include <limits>

namespace lve {

	void LVECamera::setOrthographicProjection(
		float left, float right, float top, float bottom, float near, float far) {
		projectionMatrix = glm::mat4{ 1.0f };
		projectionMatrix[0][0] = 2.f / (right - left);
		projectionMatrix[1][1] = 2.f / (bottom - top);
		projectionMatrix[2][2] = 1.f / (far - near);
		projectionMatrix[3][0] = -(right + left) / (right - left);
		projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		projectionMatrix[3][2] = -near / (far - near);
	}

	void LVECamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
		assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		const float tanHalfFovy = tan(fovy / 2.f);
		projectionMatrix = glm::mat4{ 0.0f };
		projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
		projectionMatrix[1][1] = 1.f / (tanHalfFovy);
		projectionMatrix[2][2] = far / (far - near);
		projectionMatrix[2][3] = 1.f;
		projectionMatrix[3][2] = -(far * near) / (far - near);
	}

	void LVECamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
		const glm::vec3 w{ glm::normalize(direction) };
		const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
		const glm::vec3 v{ glm::cross(w, u) };
		viewMatrix = glm::mat4{ 1.f };
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);
	}

	void LVECamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
		setViewDirection(position, target - position, up);
	}

	void LVECamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
		const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
		const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
		viewMatrix = glm::mat4{ 1.f };
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);
	}
}  // namespace lve

/*
这段代码定义了一个名为 `LVECamera` 的类中的两个函数，分别用于设置正交投影（`setOrthographicProjection`）和透视投影（`setPerspectiveProjection`）的投影矩阵。这两个投影设置用于图形编程，以进行3D渲染时的相机视图。以下是对每个函数的详细解释，包括参数的数学和几何含义。

### 1. 正交投影 (setOrthographicProjection)

函数原型：
```cpp
void LVECamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
```

#### 参数:
- **left**: 视口的左边界。
- **right**: 视口的右边界。
- **top**: 视口的上边界。
- **bottom**: 视口的下边界。
- **near**: 近裁面距离，相机到最近的渲染平面的距离。
- **far**: 远裁面距离，相机到最远的渲染平面的距离。

#### 数学含义:
正交投影将三维空间中的点投影到一个二维平面上，保持物体的相对大小，不受摄像机与物体距离的影响。它通过将三维坐标直接映射到二维坐标来实现这种效果。

#### 函数实现:
```cpp
projectionMatrix = glm::mat4{1.0f};
```
初始化为单位矩阵。

后面的几行实现了正交投影矩阵的构建：
- `projectionMatrix[0][0] = 2.f / (right - left);` 定义了在x轴方向上的缩放因子。
- `projectionMatrix[1][1] = 2.f / (bottom - top);` 定义了在y轴方向上的缩放因子。
- `projectionMatrix[2][2] = 1.f / (far - near);` 定义了在z轴方向上的缩放因子，使得深度值能够被压缩到[0, 1]范围。
- `projectionMatrix[3][0]`、`projectionMatrix[3][1]` 和 `projectionMatrix[3][2]` 用于平移，使得视口的中心对应于（0,0,0）的坐标。

### 2. 透视投影 (setPerspectiveProjection)

函数原型：
```cpp
void LVECamera::setPerspectiveProjection(float fovy, float aspect, float near, float far);
```

#### 参数:
- **fovy**: 视场角（FoV）在y轴上（以弧度为单位），表示相机的“视野”的大小。
- **aspect**: 视口的宽高比（宽度/高度），指定图像宽度与高度之比。
- **near**: 近裁面距离。
- **far**: 远裁面距离。

#### 数学含义:
透视投影模拟人眼的视野，使远处的物体看起来更小，从而产生深度感。与正交投影不同，透视投影会根据距离影响物体的视觉大小。

#### 函数实现:
```cpp
assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
```
确保宽高比不为零。

接下来，计算视角的切线值：
```cpp
const float tanHalfFovy = tan(fovy / 2.f);
```
这代表了视角的一半的切线值。

通过以下几行构建透视投影矩阵：
- `projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);` 定义了x轴的缩放因子，考虑了宽高比。
- `projectionMatrix[1][1] = 1.f / (tanHalfFovy);` 定义了y轴的缩放因子。
- `projectionMatrix[2][2] = far / (far - near);` 这一项对应的点重映射，使得深度一致。
- `projectionMatrix[2][3] = 1.f;` 设定齐次坐标，由于透视投影的特性，z值的转换至关重要。
- `projectionMatrix[3][2] = -(far * near) / (far - near);` 这个项用于设置深度偏移，将深度值适当地映射到范围内。

### 总结
这段代码通过实现两个核心函数来设置相机的投影矩阵，用于不同类型的渲染需求。正交投影保持物体的相对大小，适用于2D视图和某些3D情境，而透视投影为3D空间提供了深度和真实感。理解这些投影类型及其参数的几何和数学基础是计算机图形学中的重要部分。

*/