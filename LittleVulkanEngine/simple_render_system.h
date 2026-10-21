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
	/**
	 * 网格前向渲染系统。
	 * - 管线：sample_shader（GlobalUbo + push model/normal matrix）
	 * - 跳过 model==nullptr 的对象（如纯点光、viewer）
	 */
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
