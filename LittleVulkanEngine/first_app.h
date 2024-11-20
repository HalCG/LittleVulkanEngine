#pragma once

#include "lve_window.h"
#include "lve_pipeline.h"

namespace lve {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		void run();


	private:
		lve::LVEWindow lveWindow{ WIDTH, HEIGHT, "HelloVulkan!" };
		lve::LVEDevice lveDevice{ lveWindow };

		lve::LVEPipeline pipeline{
			lveDevice,
			"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.vert.spv",
			"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.frag.spv",
			LVEPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
			};
	};
}