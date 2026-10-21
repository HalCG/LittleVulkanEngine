#pragma once

#include "lve_device.h"
#include "lve_swap_chain.h"
#include "lve_window.h"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace lve {
	/**
	 * 帧录制与呈现的编排层。
	 * 职责：持有 SwapChain 与 per-frame command buffer；对外提供 begin/end Frame 与 RenderPass。
	 *
	 * 关键状态：
	 * - isFrameStarted：是否处于 beginFrame～endFrame 之间
	 * - currentFrameIndex：帧槽（索引 commandBuffers / 与 SwapChain::currentFrame 同步推进）
	 * - currentImageIndex：本帧 acquire 到的交换链图像下标（索引 Framebuffer）
	 */
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

		/** acquire + beginCommandBuffer；若 OUT_OF_DATE 则重建并返回 nullptr（本帧跳过） */
		VkCommandBuffer beginFrame();
		/** endCommandBuffer + submit/present；必要时重建交换链并推进 currentFrameIndex */
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
		std::vector<VkCommandBuffer> commandBuffers;  // 大小 = MAX_FRAMES_IN_FLIGHT

		uint32_t currentImageIndex;
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
}  // namespace lve
