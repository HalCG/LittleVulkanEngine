#pragma once

#include "lve_camera.h"

// lib
#include <vulkan/vulkan.h>

namespace lve {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		LVECamera& camera;
	};
}  // namespace lve
