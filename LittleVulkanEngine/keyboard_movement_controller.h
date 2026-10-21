#pragma once

#include "lve_game_object.h"
#include "lve_window.h"

namespace lve {
	/**
	 * 键盘控制 viewer（通常是无 model 的 GameObject）：
	 * WASD/EQ 平移，方向键旋转；结果写入 transform，再交给 LVECamera::setViewYXZ。
	 */
	class KeyboardMovementController {
	public:
		struct KeyMappings {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_E;
			int moveDown = GLFW_KEY_Q;
			int lookLeft = GLFW_KEY_LEFT;
			int lookRight = GLFW_KEY_RIGHT;
			int lookUp = GLFW_KEY_UP;
			int lookDown = GLFW_KEY_DOWN;
		};

		void moveInPlaneXZ(GLFWwindow* window, float dt, LVEGameObject& gameObject);

		KeyMappings keys{};
		float moveSpeed{ 3.f };
		float lookSpeed{ 1.5f };
	};
}  // namespace lve
