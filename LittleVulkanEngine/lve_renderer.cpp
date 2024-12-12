#include "lve_renderer.h"

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace lve {

	LVERenderer::LVERenderer(LVEWindow& window, LVEDevice& device)
		: lveWindow{ window }, lveDevice{ device } {
		recreateSwapChain();
		createCommandBuffers();
	}

	LVERenderer::~LVERenderer() { freeCommandBuffers(); }

	//重建交换链的方法
	//重交换链时需要检查窗口尺寸，并在设备空闲后确保任何现有资源都已正确清理。
	void LVERenderer::recreateSwapChain() {
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
			///!!! 在某些系统中，两个交换链不能在同一窗口上共存，因此这确保了旧的交换链首先被销毁或者移动。
			std::shared_ptr<LVESwapChain> oldSwapChain = std::move(lveSwapChain);
			lveSwapChain = std::make_unique<LVESwapChain>(lveDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*lveSwapChain.get())) {
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}
	}

	//分配、释放以及记录 执行时所需的一系列指令，包括设置视口、裁剪区域，以及绘制指令等。
	void LVERenderer::createCommandBuffers() {
		commandBuffers.resize(LVESwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void LVERenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(
			lveDevice.device(),
			lveDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}

	VkCommandBuffer LVERenderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress");

		auto result = lveSwapChain->acquireNextImage(&currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		return commandBuffer;
	}

	//主要功能是结束当前帧的渲染过程。
	void LVERenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress");

		//1. 获取当前的命令缓冲区（command buffer），这个缓冲区用于存储当前帧的渲染命令。
		auto commandBuffer = getCurrentCommandBuffer();
		//2. 调用 vkEndCommandBuffer 函数来结束当前的命令缓冲区记录
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		//3. 提交命令缓冲区到交换链（swap chain）进行渲染，并获取提交结果。
		auto result = lveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		
		//检查提交结果是否为交换链过期（VK_ERROR_OUT_OF_DATE_KHR）、次优（VK_SUBOPTIMAL_KHR）或窗口是否被调整大小。如果是其中之一，则需要重建交换链。
		if (result == VK_ERROR_OUT_OF_DATE_KHR 
			|| result == VK_SUBOPTIMAL_KHR 
			|| lveWindow.wasWindowResized()) 
		{
			lveWindow.resetWindowResizedFlag();
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % LVESwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void LVERenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
		assert(
			//确保传入的命令缓冲区是当前帧的命令缓冲区。
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = lveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(currentImageIndex);
		//设置渲染区域的偏移量为 (0, 0)，表示从左上角开始渲染。
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();//渲染区域的大小

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };//设置深度清除值为 1.0，表示完全清除深度缓冲区。
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, lveSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void LVERenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		//确保当前帧已经开始。如果帧未开始，程序将抛出错误信息，提示不能在帧未开始时调用该方法。
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		//确保传入的命令缓冲区是当前帧的命令缓冲区。如果不是，程序将抛出错误信息。
		assert(
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't end render pass on command buffer from a different frame");
		vkCmdEndRenderPass(commandBuffer);
	}

}  // namespace lve
