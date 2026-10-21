#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace lve {

	/**
	 * 相机：维护投影 / 视图 / 逆视图矩阵。
	 * 约定：Y 向下为正的引擎坐标系；透视深度范围 [0,1]（GLM_FORCE_DEPTH_ZERO_TO_ONE）。
	 * setViewYXZ 与 KeyboardMovementController + Transform 的 yaw/pitch 配合。
	 */
	class LVECamera {
	public:
		void setOrthographicProjection(
			float left, float right, float top, float bottom, float near, float far);

		void setPerspectiveProjection(float fovy, float aspect, float near, float far);

		void setViewDirection(
			glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewTarget(
			glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		/** rotation.x=pitch, rotation.y=yaw（与 TransformComponent 一致） */
		void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

		const glm::mat4& getProjection() const { return projectionMatrix; }
		const glm::mat4& getView() const { return viewMatrix; }
		const glm::mat4& getInverseView() const { return inverseViewMatrix; }
		/** 世界空间相机位置 = inverseView 第 4 列 xyz */
		const glm::vec3 getPosition() const { return glm::vec3(inverseViewMatrix[3]); }

	private:
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverseViewMatrix{1.f};
	};
}  // namespace lve
