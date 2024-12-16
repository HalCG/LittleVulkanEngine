#pragma once

#include "lve_device.h"
#include "lve_swap_chain.h"
#include "lve_window.h"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace lve {
	class LVERenderer {
	public:
		LVERenderer(LVEWindow& window, LVEDevice& device);
		~LVERenderer();

		LVERenderer(const LVERenderer&) = delete;
		LVERenderer& operator=(const LVERenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return lveSwapChain->getRenderPass(); }
		float getAspectRatio() const { return lveSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		LVEWindow& lveWindow;
		LVEDevice& lveDevice;
		std::unique_ptr<LVESwapChain> lveSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
}  // namespace lve
