#pragma once

#include "lve_window.h"
#include "lve_pipeline.h"
#include "lve_swap_chain.h"
#include "lve_model.h"

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

		void loadModels();

	private:
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void freeCommandBuffers();
		void drawFrame();

		//resize window 
		void recreateSwapChain();
		void recordCommandBuffer(int imageIndex);

		lve::LVEWindow lveWindow{ WIDTH, HEIGHT, "HelloVulkan!" };
		lve::LVEDevice lveDevice{ lveWindow };
		std::unique_ptr<lve::LVESwapChain> lveSwapChain;
		std::unique_ptr<LVEPipeline> lvePipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<LVEModel>  lveModel;

		//lve::LVEPipeline pipeline{
		//	lveDevice,
		//	"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.vert.spv",
		//	"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.frag.spv",
		//	LVEPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
		//	};
	};
}