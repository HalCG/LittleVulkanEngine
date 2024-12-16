#include "keyboard_movement_controller.h"

//glm
#include <glm/gtc/constants.hpp>//glm::two_pi<float>()

// std
#include <limits>

namespace lve {

	void KeyboardMovementController::moveInPlaneXZ(
		GLFWwindow* window, float dt, LVEGameObject& gameObject) {

		/// 1. 移动
		glm::vec3 rotate{ 0 };
		//计算旋转
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

		//应用旋转
		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
		}

		//限制角度
		gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
		gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

		/// 2. 旋转
		float yaw = gameObject.transform.rotation.y;
		const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
		const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		glm::vec3 moveDir{ 0.f };

		//计算移动方向
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

		//应用移动
		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		}
	}
}  // namespace lve

/*
这段代码来自一个处理键盘移动控制的类 `KeyboardMovementController`，使用 OpenGL 的 GLFW 库来处理用户输入，并通过 GLM 库处理向量和矩阵运算。代码的主要功能是控制游戏对象在一个三维平面内的移动和旋转。

以下是对代码的详细解读，特别是角度计算相关的数学几何部分：

### 1. 代码结构

- **头文件**：引入了自身的 `keyboard_movement_controller.h`，以及 GLM 和标准库 `<limits>`。
- **命名空间**：所有内容都封装在 `lve` 命名空间中，避免与其他库的命名冲突。
- **函数 `moveInPlaneXZ`**：这个函数实现了通过键盘控制游戏对象在 XZ 平面的移动和旋转。

### 2. 旋转控制

```cpp
glm::vec3 rotate{ 0 };
if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;
```

这部分代码通过检查按下的键来决定旋转的方向。旋转向量 `rotate` 定义了在 X（上下旋转）和 Y（左右旋转）方向的旋转量。根据按下的方向键，`rotate` 向量的相应坐标会增加或减少。

### 3. 旋转应用

```cpp
if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
	gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
}
```

这里使用了 `glm::dot` 来检查 `rotate` 向量是否有效（即非零向量），然后用旋转速度 `lookSpeed` 和时间步长 `dt` 乘以归一化的旋转向量，更新游戏对象的旋转。归一化是确保旋转不会因旋转量的大小而放大或缩小。

### 4. 限制角度

```cpp
gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
```

- 对 `x`（俯仰角）进行限制，使其在 -1.5 到 1.5 弧度之间，基本上相当于约 ±85 度。这是为了限制视角，防止用户翻转或过度抬头。
- 使用 `glm::mod` 函数将 `y`（偏航角）限制在 0 到 2π 范围内，确保偏航角是合法的（即保持在一个完整的360度旋转内）。

### 5. 计算移动方向

```cpp
float yaw = gameObject.transform.rotation.y;
const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
const glm::vec3 upDir{ 0.f, -1.f, 0.f };
```

- `yaw` 表示游戏对象当前的偏航角（绕 Y 轴的旋转），通过正弦和余弦函数计算出前进方向的向量 `forwardDir`。
- 前进方向 `forwardDir` 定义为 `(sin(yaw), 0, cos(yaw))`，这是因为我们只在 XZ 平面上移动，Y 值固定为 0。
- 右侧方向向量 `rightDir` 的计算是通过取 `forwardDir` 的正交向量得到的，表示向右的移动方向。
- `upDir` 定义为 `(0, -1, 0)`，表示向上的方向（相对于负 Y 方向），通常用于处理上下移动。

### 6. 移动控制

```cpp
glm::vec3 moveDir{ 0.f };
if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
```

这一部分根据按下的键来决定移动方向向量 `moveDir`。每个方向的按键更新对应的 `moveDir` 向量。

### 7. 应用移动

```cpp
if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
	gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
}
```

最后，类似旋转的处理方式，检查 `moveDir` 是否有效，然后使用移动速度 `moveSpeed` 和时间步长 `dt` 更新游戏对象的位置。

### 总结

这段代码实现了一个简单的键盘控制系统，允许用户通过方向键控制物体在 XZ 平面上的旋转和移动。通过使用三角函数、向量运算以及适当的角度限制，确保了物体移动的自然和有效。你可以根据需要扩展或修改这个控制器，以适应不同的游戏需求。

*/