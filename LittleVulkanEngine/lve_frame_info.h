#pragma once

#include "lve_camera.h"
#include "lve_game_object.h"

// lib
#include <vulkan/vulkan.h>

namespace lve {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		LVECamera& camera;
		VkDescriptorSet globalDescriptorSet;
		LVEGameObject::Map& gameObjects;
	};
}  // namespace lve
