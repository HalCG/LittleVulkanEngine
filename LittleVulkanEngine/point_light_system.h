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
	 * 点光源系统。
	 * update()：旋转灯光位置，写入 GlobalUbo.pointLights。
	 * render()：无顶点缓冲的 Billboard（point_light shader + alpha blend），按距相机远近排序。
	 */
	class PointLightSystem {
	public:
		PointLightSystem(
			LVEDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		void update(FrameInfo& frameInfo, GlobalUbo& ubo);
		void render(FrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		LVEDevice& lveDevice;

		std::unique_ptr<LVEPipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
	};
}  // namespace lve
