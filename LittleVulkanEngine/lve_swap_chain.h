#pragma once

#include "lve_device.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>

namespace lve {

	/**
	 * 交换链及相关资源。
	 * 职责：可呈现图像、ImageView、深度图、RenderPass、Framebuffer、帧同步对象。
	 *
	 * 关键状态：
	 * - currentFrame：CPU 帧槽 [0, MAX_FRAMES_IN_FLIGHT)，索引 imageAvailable / fence
	 * - imageIndex：交换链图像下标，索引 Framebuffer 与 renderFinished semaphore
	 * - oldSwapChain：重建时短暂持有旧链，创建完成后置空
	 *
	 * MAX_FRAMES_IN_FLIGHT=2：允许 CPU 录下一帧时 GPU 仍在画上一帧。
	 * renderFinished 必须「每张 swap 图一个」：present 会占用该 semaphore，
	 * 直到该图再次 acquire；若按帧槽复用（旧教程写法）会在 3 缓冲时校验失败甚至卡死。
	 */
	class LVESwapChain {
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		LVESwapChain(LVEDevice& deviceRef, VkExtent2D windowExtent);
		/** previous 非空时，createInfo.oldSwapchain 指向旧链，便于驱动复用资源 */
		LVESwapChain(LVEDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<LVESwapChain> previous);
		~LVESwapChain();

		LVESwapChain(const LVESwapChain&) = delete;
		LVESwapChain& operator=(const LVESwapChain&) = delete;

		VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
		VkRenderPass getRenderPass() { return renderPass; }
		VkImageView getImageView(int index) { return swapChainImageViews[index]; }
		size_t imageCount() { return swapChainImages.size(); }
		VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
		VkExtent2D getSwapChainExtent() { return swapChainExtent; }
		uint32_t width() { return swapChainExtent.width; }
		uint32_t height() { return swapChainExtent.height; }

		float extentAspectRatio() {
			return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
		}
		VkFormat findDepthFormat();

		/** 等待本 frame 槽 fence → acquire 下一张可呈现 image，信号 imageAvailable semaphore */
		VkResult acquireNextImage(uint32_t* imageIndex);
		/** 等待 imageAvailable → submit 图形队列 → present；推进 currentFrame */
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

		bool compareSwapFormats(const LVESwapChain& swapChain) const {
			return	swapChain.swapChainDepthFormat == swapChainDepthFormat &&
					swapChain.swapChainImageFormat == swapChainImageFormat;
		}

	private:
		void init();
		void createSwapChain();
		void createImageViews();
		void createDepthResources();
		void createRenderPass();
		void createFramebuffers();
		void createSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(
			const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat swapChainImageFormat;
		VkFormat swapChainDepthFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkRenderPass renderPass;

		std::vector<VkImage> depthImages;
		std::vector<VkDeviceMemory> depthImageMemorys;
		std::vector<VkImageView> depthImageViews;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		LVEDevice& device;
		VkExtent2D windowExtent;

		VkSwapchainKHR swapChain;
		std::shared_ptr<LVESwapChain> oldSwapChain;

		std::vector<VkSemaphore> imageAvailableSemaphores;  // 大小=MAX_FRAMES_IN_FLIGHT，按帧槽
		std::vector<VkSemaphore> renderFinishedSemaphores;  // 大小=imageCount，按 imageIndex（供 present）
		std::vector<VkFence> inFlightFences;                // 按帧槽：CPU 等 GPU
		std::vector<VkFence> imagesInFlight;                // 按 image：防止同一图被两帧共用
		size_t currentFrame = 0;
	};

}  // namespace lve
