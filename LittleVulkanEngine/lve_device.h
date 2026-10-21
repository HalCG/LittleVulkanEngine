#pragma once

#include "lve_window.h"

// std lib headers
#include <string>
#include <vector>

namespace lve {

	/** 查询交换链创建所需的 surface 能力、格式与呈现模式 */
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/** 图形队列族与呈现队列族索引（可能相同） */
	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t presentFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
	};

	/**
	 * Vulkan 设备层封装。
	 * 职责：Instance / Surface / 物理与逻辑设备 / 队列 / 默认 CommandPool，
	 *       以及创建 Buffer、Image、单次提交命令等辅助函数。
	 * 构造顺序：Instance → DebugMessenger → Surface → 选 GPU → LogicalDevice → CommandPool。
	 */
	class LVEDevice {
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		LVEDevice(LVEWindow& window);
		~LVEDevice();

		// Not copyable or movable
		LVEDevice(const LVEDevice&) = delete;
		void operator=(const LVEDevice&) = delete;
		LVEDevice(LVEDevice&&) = delete;
		LVEDevice& operator=(LVEDevice&&) = delete;

		VkCommandPool getCommandPool() { return commandPool; }
		VkDevice device() { return device_; }
		VkSurfaceKHR surface() { return surface_; }
		VkQueue graphicsQueue() { return graphicsQueue_; }
		VkQueue presentQueue() { return presentQueue_; }

		SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
		/** typeFilter 来自 memoryRequirements；properties 如 HOST_VISIBLE / DEVICE_LOCAL */
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
		VkFormat findSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// Buffer / Image 辅助
		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(
			VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void createImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);

		VkPhysicalDeviceProperties properties;

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void hasGflwRequiredInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		LVEWindow& window;
		VkCommandPool commandPool;  // 供短生命周期一次性命令与 Renderer 主 command buffer

		VkDevice device_;
		VkSurfaceKHR surface_;
		VkQueue graphicsQueue_;
		VkQueue presentQueue_;

		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};

}  // namespace lve
