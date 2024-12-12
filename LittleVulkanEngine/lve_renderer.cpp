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

	//�ؽ��������ķ���
	//�ؽ�����ʱ��Ҫ��鴰�ڳߴ磬�����豸���к�ȷ���κ�������Դ������ȷ����
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
			///!!! ��ĳЩϵͳ�У�����������������ͬһ�����Ϲ��棬�����ȷ���˾ɵĽ��������ȱ����ٻ����ƶ���
			std::shared_ptr<LVESwapChain> oldSwapChain = std::move(lveSwapChain);
			lveSwapChain = std::make_unique<LVESwapChain>(lveDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*lveSwapChain.get())) {
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}
	}

	//���䡢�ͷ��Լ���¼ ִ��ʱ�����һϵ��ָ����������ӿڡ��ü������Լ�����ָ��ȡ�
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

	//��Ҫ�����ǽ�����ǰ֡����Ⱦ���̡�
	void LVERenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress");

		//1. ��ȡ��ǰ�����������command buffer����������������ڴ洢��ǰ֡����Ⱦ���
		auto commandBuffer = getCurrentCommandBuffer();
		//2. ���� vkEndCommandBuffer ������������ǰ�����������¼
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		//3. �ύ�����������������swap chain��������Ⱦ������ȡ�ύ�����
		auto result = lveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		
		//����ύ����Ƿ�Ϊ���������ڣ�VK_ERROR_OUT_OF_DATE_KHR�������ţ�VK_SUBOPTIMAL_KHR���򴰿��Ƿ񱻵�����С�����������֮һ������Ҫ�ؽ���������
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
			//ȷ���������������ǵ�ǰ֡�����������
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = lveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(currentImageIndex);
		//������Ⱦ�����ƫ����Ϊ (0, 0)����ʾ�����Ͻǿ�ʼ��Ⱦ��
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();//��Ⱦ����Ĵ�С

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };//����������ֵΪ 1.0����ʾ��ȫ�����Ȼ�������
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
		//ȷ����ǰ֡�Ѿ���ʼ�����֡δ��ʼ�������׳�������Ϣ����ʾ������֡δ��ʼʱ���ø÷�����
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		//ȷ���������������ǵ�ǰ֡�����������������ǣ������׳�������Ϣ��
		assert(
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't end render pass on command buffer from a different frame");
		vkCmdEndRenderPass(commandBuffer);
	}

}  // namespace lve
