#pragma once

#include "lve_camera.h"
#include "lve_game_object.h"

// lib
#include <vulkan/vulkan.h>

namespace lve {

#define MAX_LIGHTS 10

	/** 与 GLSL PointLight 布局对应；color.w 为强度 */
	struct PointLight {
		glm::vec4 position{};  // ignore w
		glm::vec4 color{};     // w is intensity
	};

	/**
	 * 每帧上传的全局 UBO（set=0, binding=0）。
	 * 字段顺序/对齐必须与 sample_shader / point_light 中 GlobalUbo 一致。
	 */
	struct GlobalUbo {
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{1.f};  // GLSL 名 invView；[3].xyz 为相机世界坐标
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };  // w is intensity
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	/**
	 * 单帧渲染上下文，传给各 RenderSystem。
	 * frameIndex 选 per-frame UBO/descriptor；commandBuffer 用于录制；gameObjects 为场景表。
	 */
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		LVECamera& camera;
		VkDescriptorSet globalDescriptorSet;
		LVEGameObject::Map& gameObjects;
	};
}  // namespace lve
