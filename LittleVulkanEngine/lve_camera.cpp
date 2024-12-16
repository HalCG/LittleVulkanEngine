#include "lve_camera.h"

// std
#include <cassert>
#include <limits>

namespace lve {

void LVECamera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) {
  projectionMatrix = glm::mat4{1.0f};
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
  projectionMatrix = glm::mat4{0.0f};
  projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
  projectionMatrix[1][1] = 1.f / (tanHalfFovy);
  projectionMatrix[2][2] = far / (far - near);
  projectionMatrix[2][3] = 1.f;
  projectionMatrix[3][2] = -(far * near) / (far - near);
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