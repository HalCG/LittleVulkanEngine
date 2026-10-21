#pragma once
#include "lve_model.h"

// std
#include <memory>
#include <unordered_map>

namespace lve {
	/** TRS：mat4 = T * Ry * Rx * Rz * S（YXZ Tait-Bryan） */
	struct TransformComponent {
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f, 1.f, 1.f };
		glm::vec3 rotation{};

		glm::mat4 mat4();
		/** 法线矩阵（含非均匀缩放的逆转置简化形式） */
		glm::mat3 normalMatrix();
	};

	struct PointLightComponent {
		float lightIntensity = 1.0f;
	};

	/**
	 * 轻量场景实体（非完整 ECS）。
	 * - 有 model：网格物体（SimpleRenderSystem 绘制）
	 * - 有 pointLight：点光（写入 UBO + PointLightSystem Billboard）
	 * Map 以 id 为键，便于按 id 查找/排序。
	 */
	class LVEGameObject {
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, LVEGameObject>;

		static LVEGameObject createGameObject() {
			static id_t currentId = 0;
			return LVEGameObject{ currentId++ };
		}

		/** scale.x 用作灯光球 radius；color 为灯光颜色 */
		static LVEGameObject makePointLight(
			float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

		LVEGameObject(const LVEGameObject&) = delete;
		LVEGameObject& operator=(const LVEGameObject&) = delete;
		LVEGameObject(LVEGameObject&&) = default;
		LVEGameObject& operator=(LVEGameObject&&) = default;

		id_t getId() { return id; }
		glm::vec3 color{};
		TransformComponent transform{};

		// 可选组件
		std::shared_ptr<LVEModel> model{};
		std::unique_ptr<PointLightComponent> pointLight = nullptr;

	private:
		LVEGameObject(id_t objId) : id{ objId } {}
		id_t id;
	};
}  // namespace lve
