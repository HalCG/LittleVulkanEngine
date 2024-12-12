#include "first_app.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <array>

namespace lve {

	struct SimplePushConstantData {
		glm::mat2 transform{1.f};//{{1.f,  0.f},{0.f,  1.f}}
		glm::vec2 offset;
		alignas(16) glm::vec3 color;//三角形从红色->蓝色. alignas(16) 来确保数据按照 16 字节对齐，满足 Vulkan 对内存对齐要求。
	};

	FirstApp::FirstApp() {
		loadGameObjects();
		createPipelineLayout();
		//createPipeline();
		recreateSwapChain();//在交换链创建后创建pipeline
		createCommandBuffers();
	}

	FirstApp::~FirstApp() {
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	void FirstApp::run() {
		while (!lveWindow.shouldClose()) {
			glfwPollEvents();
			drawFrame();// 绘制当前帧 
		}

		vkDeviceWaitIdle(lveDevice.device());
	}

	void FirstApp::loadGameObjects() {
		std::vector<LVEModel::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},//没有color属性的时候默认是黑色
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};

		auto lveModel = std::make_shared<LVEModel>(lveDevice, vertices);
		auto triangle = LVEGameObject::createGameObject();
		triangle.model = lveModel;
		triangle.color = { .1f, .8f, .1f };
		triangle.transform2d.translation.x = .2f;						//平移
		triangle.transform2d.scale = { 2.f, .5f};						//缩放
		triangle.transform2d.rotation = .25f * glm::two_pi<float>();	//旋转
		gameObjects.push_back(std::move(triangle));
	}

	//创建了一个包含推送常量范围的管线布局，以便在着色器之间传递动态数据。
	void FirstApp::createPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(
			lveDevice.device(), 
			&pipelineLayoutInfo, 
			nullptr, 
			&pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	//重建交换链的方法
	//重交换链时需要检查窗口尺寸，并在设备空闲后确保任何现有资源都已正确清理。
	void  FirstApp::recreateSwapChain() {
		auto extent = lveWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = lveWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(lveDevice.device());

		if (lveSwapChain == nullptr) {
			lveSwapChain = std::make_unique<LVESwapChain>(lveDevice, extent);
		}
		else {
			//lveSwapChain = nullptr;///!!! 在某些系统中，两个交换链不能在同一窗口上共存，因此这确保了旧的交换链首先被销毁。
			lveSwapChain = std::make_unique<LVESwapChain>(lveDevice, extent, std::move(lveSwapChain));
			
			if (lveSwapChain->imageCount() != commandBuffers.size()) {
				freeCommandBuffers();
				createCommandBuffers();
			}
		}

		// if render pass compatible do nothing else:
		createPipeline();
	}

	//配置并创建图形渲染所需的渲染管道，包括指定着色器文件路径和其他设置。
	void FirstApp::createPipeline() {
		assert(lveSwapChain != nullptr && "Cannot create pipeline before swap chain");
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		LVEPipeline::defaultPipelineConfigInfo(pipelineConfig);

		//这样头文件中构造lvePipeline的错误就没了
		pipelineConfig.renderPass = lveSwapChain->getRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;

		lvePipeline = std::make_unique<LVEPipeline>(
			lveDevice,
			//"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.vert.spv",
			//"E:/vulkan/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.frag.spv",
			"E:/opengl/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.vert.spv",
			"E:/opengl/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.frag.spv",
			pipelineConfig
			);
	}

	//分配、释放以及记录 执行时所需的一系列指令，包括设置视口、裁剪区域，以及绘制指令等。
	void FirstApp::createCommandBuffers() {
		commandBuffers.resize(lveSwapChain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data())) {
			throw std::runtime_error("failed to allocae command buffers!");
		}
	}

	void FirstApp::freeCommandBuffers() {
		vkFreeCommandBuffers(lveDevice.device(), lveDevice.getCommandPool(), static_cast<float>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	//录制过程中还会利用推送常量来动态改变三角形的位置和颜色
	void FirstApp::recordCommandBuffer(int imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};

		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = lveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(imageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f,0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, static_cast<uint32_t>(0.0f) };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{ {0,0}, lveSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

		renderGameObjects(commandBuffers[imageIndex]);
		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) {
		lvePipeline->bind(commandBuffer);

		for (auto& obj : gameObjects) {
			obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.001f, glm::two_pi<float>());

			SimplePushConstantData pushConstantData{};
			pushConstantData.offset = obj.transform2d.translation;
			pushConstantData.color = obj.color;
			pushConstantData.transform = obj.transform2d.mat2();

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&pushConstantData);

			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}

	void FirstApp::drawFrame(){
		//1. 获取图像索引：调用交换链（swap chain）以获取下一个可用图像。
		uint32_t imageIndex;
		auto result = lveSwapChain->acquireNextImage(&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		//2. 提交命令缓冲区：如果成功获取到图像，会使用命令缓冲区向 GPU 提交渲染命令。
		recordCommandBuffer(imageIndex);
		result = lveSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResize()) {
			lveWindow.resetWindowResizeFlag();
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}
};