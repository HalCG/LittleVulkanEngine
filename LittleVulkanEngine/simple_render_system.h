#pragma once

#include "lve_camera.h"
#include "lve_device.h"
#include "lve_game_object.h"
#include "lve_pipeline.h"

// std
#include <memory>
#include <vector>

namespace lve {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(LVEDevice& device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(
			VkCommandBuffer commandBuffer,
			std::vector<LVEGameObject>& gameObjects,
			const LVECamera& camera);
	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);

		LVEDevice& lveDevice;

		std::unique_ptr<LVEPipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
	};
}  // namespace lve
