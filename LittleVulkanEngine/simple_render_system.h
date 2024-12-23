#pragma once

#include "lve_camera.h"
#include "lve_device.h"
#include "lve_frame_info.h"
#include "lve_game_object.h"
#include "lve_pipeline.h"

// std
#include <memory>
#include <vector>

namespace lve {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(
			LVEDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(FrameInfo& frameInfo);
	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		LVEDevice& lveDevice;

		std::unique_ptr<LVEPipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
	};
}  // namespace lve
