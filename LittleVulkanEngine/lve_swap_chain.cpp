#include "lve_swap_chain.h"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace lve {

	LVESwapChain::LVESwapChain(LVEDevice& deviceRef, VkExtent2D extent)
		: device{ deviceRef }, windowExtent{ extent } {
		init();
	}

	LVESwapChain::LVESwapChain(LVEDevice& deviceRef, VkExtent2D extent, std::shared_ptr<LVESwapChain> previous)
		: device{ deviceRef }, windowExtent{ extent }, oldSwapChain{ previous } {
		init();

		// 新链已创建完毕，释放对旧链的 shared_ptr（旧资源在此时销毁）
		oldSwapChain = nullptr;
	}

	void LVESwapChain::init() {
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDepthResources();
		createFramebuffers();
		createSyncObjects();
	}

	LVESwapChain::~LVESwapChain() {
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device.device(), imageView, nullptr);
		}
		swapChainImageViews.clear();

		if (swapChain != nullptr) {
			vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
			swapChain = nullptr;
		}

		for (int i = 0; i < depthImages.size(); i++) {
			vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
			vkDestroyImage(device.device(), depthImages[i], nullptr);
			vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
		}

		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
		}

		vkDestroyRenderPass(device.device(), renderPass, nullptr);

		// cleanup synchronization objects
		for (auto semaphore : renderFinishedSemaphores) {
			vkDestroySemaphore(device.device(), semaphore, nullptr);
		}
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device.device(), inFlightFences[i], nullptr);
		}
	}

	// 取得下一张可画的 swap image 下标；CPU 先等本帧槽上一轮 GPU 做完
	VkResult LVESwapChain::acquireNextImage(uint32_t* imageIndex) {
		// fence：CPU↔GPU。VK_TRUE=等全部；timeout=max 表示一直等
		vkWaitForFences(
			device.device(),
			1,
			&inFlightFences[currentFrame],
			VK_TRUE,
			std::numeric_limits<uint64_t>::max());

		// timeout=max：一直等到有图；成功后 signal imageAvailable（GPU 侧）
		// 第 5 参 fence 这里不用（传 NULL），用 semaphore 串后续 submit
		VkResult result = vkAcquireNextImageKHR(
			device.device(),
			swapChain,
			std::numeric_limits<uint64_t>::max(),
			imageAvailableSemaphores[currentFrame],  // 必须是未发出信号的 semaphore
			VK_NULL_HANDLE,
			imageIndex);

		return result;
	}

	// 提交录制好的 command buffer，并 present 到屏幕
	VkResult LVESwapChain::submitCommandBuffers(
		const VkCommandBuffer* buffers, uint32_t* imageIndex)
	{
		// 若这张 swap image 仍被更早的帧占用，先等那个 fence
		if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// 等到「颜色附件输出」阶段才真正执行（与 acquire 的 imageAvailable 配对）
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;  // 在哪些管线阶段才开始等结束

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		// 按 imageIndex 取 semaphore：present 会一直占用它，直到该图再次被 acquire
		// （不可按 currentFrame 复用，否则 3 张图 + 2 帧槽时会 signal 仍在 present 中的 semaphore）
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[*imageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// 提交前把 fence 置未信号；submit 完成后再信号，供下一轮 wait
		vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;  // 等渲染完成再显示

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = imageIndex;  // 要显示的那张 image 下标

		auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	void LVESwapChain::createSwapChain() {
		SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// 比驱动最小值多要 1 张，减少等图阻塞；再夹到 maxImageCount（0 表示无上限）
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.surface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;           // 如 B8G8R8A8_SRGB
		createInfo.imageColorSpace = surfaceFormat.colorSpace; // 如 SRGB_NONLINEAR
		createInfo.imageExtent = extent;                         // 像素宽高
		createInfo.imageArrayLayers = 1;                         // 2D 非立体则为 1
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // 当作颜色渲染目标

		QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily) {
			// 图形与呈现不在同一队列族：图像需并发共享
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;  // 独占，更高效
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;  // 通常 IDENTITY
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // 与窗口合成：不透明

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;  // 被其它窗口挡住的像素可不渲染

		// 重建时传入旧链，便于驱动复用/平滑过渡
		createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

		if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// minImageCount 只是下限；实际张数可能更多 → 先查数量再取句柄
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// Image 是原始像素存储；ImageView 告诉管线「按什么类型/aspect 解读」
	void LVESwapChain::createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = swapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // 颜色面（非深度）
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;   // 无 mip 链
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) !=
				VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}
		}
	}

	// RenderPass：描述附件「开始怎么处理、结束变成什么布局、subpass 怎么用它们」
	void LVESwapChain::createRenderPass() {
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;           // 无 MSAA
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;     // 开始时清深度
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // 结束后不需要保留深度
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // 不保留旧内容
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;  // 对应 attachments 数组下标 1
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = getSwapChainImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // 清成 beginRenderPass 的 clearValue
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 要拿去 present，必须保留
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // 结束后可呈现

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;  // attachments[0]
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // subpass 内布局

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;  // 对应 shader 的 layout(location=0) out
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		// 外部（上一帧 present / 布局转换）→ 本 subpass 写颜色/深度 之前的依赖
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // 隐式「外部」subpass
		dependency.srcAccessMask = 0;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	// Framebuffer = 某次 RenderPass 实际绑到的 ImageView 集合（颜色 + 深度），每张 swap image 一个
	void LVESwapChain::createFramebuffers() {
		swapChainFramebuffers.resize(imageCount());
		for (size_t i = 0; i < imageCount(); i++) {
			std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageViews[i] };

			VkExtent2D swapChainExtent = getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;  // 必须与创建时兼容的那个 RenderPass
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();  // 顺序对应 RenderPass 附件下标
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(
				device.device(),
				&framebufferInfo,
				nullptr,
				&swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	// 深度图：与每张颜色 swap image 同尺寸；DEVICE_LOCAL；aspect=DEPTH
	void LVESwapChain::createDepthResources() {
		VkFormat depthFormat = findDepthFormat();
		swapChainDepthFormat = depthFormat;
		VkExtent2D swapChainExtent = getSwapChainExtent();

		depthImages.resize(imageCount());
		depthImageMemorys.resize(imageCount());
		depthImageViews.resize(imageCount());

		for (int i = 0; i < depthImages.size(); i++) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;       // 2D 图 depth 维为 1
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat; // D32 / D24_S8 等
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;  // GPU 最优排布，CPU 一般不直接读
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			device.createImageWithInfo(
				imageInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImages[i],
				depthImageMemorys[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture image view!");
			}
		}
	}

	// imageAvailable / fence：按帧槽；renderFinished：按 swap 图像数（见 Vulkan Guide swapchain_semaphore_reuse）
	void LVESwapChain::createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(imageCount());
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// 初始已信号：否则首帧 acquire 前 waitFence 会永远卡住
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create per-frame sync objects!");
			}
		}
		for (size_t i = 0; i < imageCount(); i++) {
			if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
				VK_SUCCESS) {
				throw std::runtime_error("failed to create per-image renderFinished semaphore!");
			}
		}
	}

	// 优先 sRGB 格式：显示更符合人眼；UNORM 是线性空间，适合中间计算纹理
	VkSurfaceFormatKHR LVESwapChain::chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return availableFormats[0];  // 退化：用列表第一个
	}

	// MAILBOX≈三缓冲低延迟；没有则 FIFO（保证支持的 VSync）
	VkPresentModeKHR LVESwapChain::chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "Present mode: Mailbox" << std::endl;
				return availablePresentMode;
			}
		}
		std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// 多数平台 currentExtent 已固定为窗口大小；若为 special max 值则需自己 clamp
	VkExtent2D LVESwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = windowExtent;
			actualExtent.width = std::max(
				capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(
				capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	// 按优先级找「能当深度附件 + OPTIMAL tiling」的格式
	VkFormat LVESwapChain::findDepthFormat() {
		return device.findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

}  // namespace lve
