#pragma once

#include "lve_window.h"
#include "lve_pipeline.h"
#include "lve_swap_chain.h"

//std
#include <memory>
#include <vector>

namespace lve {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void drawFrame();

		lve::LVEWindow lveWindow{ WIDTH, HEIGHT, "HelloVulkan!" };
		lve::LVEDevice lveDevice{ lveWindow };
		lve::LVESwapChain lveSwapChain{lveDevice, lveWindow.getExtent()};
		std::unique_ptr<LVEPipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;

		//lve::LVEPipeline pipeline{
		//	lveDevice,
		//	"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.vert.spv",
		//	"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.frag.spv",
		//	LVEPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
		//	};
	};
}