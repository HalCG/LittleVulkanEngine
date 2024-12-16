#include "keyboard_movement_controller.h"

//glm
#include <glm/gtc/constants.hpp>//glm::two_pi<float>()

// std
#include <limits>

namespace lve {

	void KeyboardMovementController::moveInPlaneXZ(
		GLFWwindow* window, float dt, LVEGameObject& gameObject) {

		/// 1. �ƶ�
		glm::vec3 rotate{ 0 };
		//������ת
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

		//Ӧ����ת
		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
		}

		//���ƽǶ�
		gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
		gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

		/// 2. ��ת
		float yaw = gameObject.transform.rotation.y;
		const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
		const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		glm::vec3 moveDir{ 0.f };

		//�����ƶ�����
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

		//Ӧ���ƶ�
		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		}
	}
}  // namespace lve

/*
��δ�������һ����������ƶ����Ƶ��� `KeyboardMovementController`��ʹ�� OpenGL �� GLFW ���������û����룬��ͨ�� GLM �⴦�������;������㡣�������Ҫ�����ǿ�����Ϸ������һ����άƽ���ڵ��ƶ�����ת��

�����ǶԴ������ϸ������ر��ǽǶȼ�����ص���ѧ���β��֣�

### 1. ����ṹ

- **ͷ�ļ�**������������� `keyboard_movement_controller.h`���Լ� GLM �ͱ�׼�� `<limits>`��
- **�����ռ�**���������ݶ���װ�� `lve` �����ռ��У��������������������ͻ��
- **���� `moveInPlaneXZ`**���������ʵ����ͨ�����̿�����Ϸ������ XZ ƽ����ƶ�����ת��

### 2. ��ת����

```cpp
glm::vec3 rotate{ 0 };
if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;
```

�ⲿ�ִ���ͨ����鰴�µļ���������ת�ķ�����ת���� `rotate` �������� X��������ת���� Y��������ת���������ת�������ݰ��µķ������`rotate` ��������Ӧ��������ӻ���١�

### 3. ��תӦ��

```cpp
if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
	gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
}
```

����ʹ���� `glm::dot` ����� `rotate` �����Ƿ���Ч����������������Ȼ������ת�ٶ� `lookSpeed` ��ʱ�䲽�� `dt` ���Թ�һ������ת������������Ϸ�������ת����һ����ȷ����ת��������ת���Ĵ�С���Ŵ����С��

### 4. ���ƽǶ�

```cpp
gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
```

- �� `x`�������ǣ��������ƣ�ʹ���� -1.5 �� 1.5 ����֮�䣬�������൱��Լ ��85 �ȡ�����Ϊ�������ӽǣ���ֹ�û���ת�����̧ͷ��
- ʹ�� `glm::mod` ������ `y`��ƫ���ǣ������� 0 �� 2�� ��Χ�ڣ�ȷ��ƫ�����ǺϷ��ģ���������һ��������360����ת�ڣ���

### 5. �����ƶ�����

```cpp
float yaw = gameObject.transform.rotation.y;
const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
const glm::vec3 upDir{ 0.f, -1.f, 0.f };
```

- `yaw` ��ʾ��Ϸ����ǰ��ƫ���ǣ��� Y �����ת����ͨ�����Һ����Һ��������ǰ����������� `forwardDir`��
- ǰ������ `forwardDir` ����Ϊ `(sin(yaw), 0, cos(yaw))`��������Ϊ����ֻ�� XZ ƽ�����ƶ���Y ֵ�̶�Ϊ 0��
- �Ҳ෽������ `rightDir` �ļ�����ͨ��ȡ `forwardDir` �����������õ��ģ���ʾ���ҵ��ƶ�����
- `upDir` ����Ϊ `(0, -1, 0)`����ʾ���ϵķ�������ڸ� Y ���򣩣�ͨ�����ڴ��������ƶ���

### 6. �ƶ�����

```cpp
glm::vec3 moveDir{ 0.f };
if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
```

��һ���ָ��ݰ��µļ��������ƶ��������� `moveDir`��ÿ������İ������¶�Ӧ�� `moveDir` ������

### 7. Ӧ���ƶ�

```cpp
if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
	gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
}
```

���������ת�Ĵ���ʽ����� `moveDir` �Ƿ���Ч��Ȼ��ʹ���ƶ��ٶ� `moveSpeed` ��ʱ�䲽�� `dt` ������Ϸ�����λ�á�

### �ܽ�

��δ���ʵ����һ���򵥵ļ��̿���ϵͳ�������û�ͨ����������������� XZ ƽ���ϵ���ת���ƶ���ͨ��ʹ�����Ǻ��������������Լ��ʵ��ĽǶ����ƣ�ȷ���������ƶ�����Ȼ����Ч������Ը�����Ҫ��չ���޸����������������Ӧ��ͬ����Ϸ����

*/