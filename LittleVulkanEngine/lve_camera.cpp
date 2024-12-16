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
��δ��붨����һ����Ϊ `LVECamera` �����е������������ֱ�������������ͶӰ��`setOrthographicProjection`����͸��ͶӰ��`setPerspectiveProjection`����ͶӰ����������ͶӰ��������ͼ�α�̣��Խ���3D��Ⱦʱ�������ͼ�������Ƕ�ÿ����������ϸ���ͣ�������������ѧ�ͼ��κ��塣

### 1. ����ͶӰ (setOrthographicProjection)

����ԭ�ͣ�
```cpp
void LVECamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
```

#### ����:
- **left**: �ӿڵ���߽硣
- **right**: �ӿڵ��ұ߽硣
- **top**: �ӿڵ��ϱ߽硣
- **bottom**: �ӿڵ��±߽硣
- **near**: ��������룬������������Ⱦƽ��ľ��롣
- **far**: Զ������룬�������Զ����Ⱦƽ��ľ��롣

#### ��ѧ����:
����ͶӰ����ά�ռ��еĵ�ͶӰ��һ����άƽ���ϣ������������Դ�С���������������������Ӱ�졣��ͨ������ά����ֱ��ӳ�䵽��ά������ʵ������Ч����

#### ����ʵ��:
```cpp
projectionMatrix = glm::mat4{1.0f};
```
��ʼ��Ϊ��λ����

����ļ���ʵ��������ͶӰ����Ĺ�����
- `projectionMatrix[0][0] = 2.f / (right - left);` ��������x�᷽���ϵ��������ӡ�
- `projectionMatrix[1][1] = 2.f / (bottom - top);` ��������y�᷽���ϵ��������ӡ�
- `projectionMatrix[2][2] = 1.f / (far - near);` ��������z�᷽���ϵ��������ӣ�ʹ�����ֵ�ܹ���ѹ����[0, 1]��Χ��
- `projectionMatrix[3][0]`��`projectionMatrix[3][1]` �� `projectionMatrix[3][2]` ����ƽ�ƣ�ʹ���ӿڵ����Ķ�Ӧ�ڣ�0,0,0�������ꡣ

### 2. ͸��ͶӰ (setPerspectiveProjection)

����ԭ�ͣ�
```cpp
void LVECamera::setPerspectiveProjection(float fovy, float aspect, float near, float far);
```

#### ����:
- **fovy**: �ӳ��ǣ�FoV����y���ϣ��Ի���Ϊ��λ������ʾ����ġ���Ұ���Ĵ�С��
- **aspect**: �ӿڵĿ�߱ȣ����/�߶ȣ���ָ��ͼ������߶�֮�ȡ�
- **near**: ��������롣
- **far**: Զ������롣

#### ��ѧ����:
͸��ͶӰģ�����۵���Ұ��ʹԶ�������忴������С���Ӷ�������ȸС�������ͶӰ��ͬ��͸��ͶӰ����ݾ���Ӱ��������Ӿ���С��

#### ����ʵ��:
```cpp
assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
```
ȷ����߱Ȳ�Ϊ�㡣

�������������ӽǵ�����ֵ��
```cpp
const float tanHalfFovy = tan(fovy / 2.f);
```
��������ӽǵ�һ�������ֵ��

ͨ�����¼��й���͸��ͶӰ����
- `projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);` ������x����������ӣ������˿�߱ȡ�
- `projectionMatrix[1][1] = 1.f / (tanHalfFovy);` ������y����������ӡ�
- `projectionMatrix[2][2] = far / (far - near);` ��һ���Ӧ�ĵ���ӳ�䣬ʹ�����һ�¡�
- `projectionMatrix[2][3] = 1.f;` �趨������꣬����͸��ͶӰ�����ԣ�zֵ��ת��������Ҫ��
- `projectionMatrix[3][2] = -(far * near) / (far - near);` ����������������ƫ�ƣ������ֵ�ʵ���ӳ�䵽��Χ�ڡ�

### �ܽ�
��δ���ͨ��ʵ���������ĺ��������������ͶӰ�������ڲ�ͬ���͵���Ⱦ��������ͶӰ�����������Դ�С��������2D��ͼ��ĳЩ3D�龳����͸��ͶӰΪ3D�ռ��ṩ����Ⱥ���ʵ�С������ЩͶӰ���ͼ�������ļ��κ���ѧ�����Ǽ����ͼ��ѧ�е���Ҫ���֡�

*/