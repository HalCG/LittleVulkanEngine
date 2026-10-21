#pragma once

#define GLFW_INCLUDE_VULKAN

#include <string>
#include <GLFW/glfw3.h>

namespace lve {
	/**
	 * GLFW 窗口封装。
	 * 职责：创建原生窗口、转发 framebuffer 尺寸变化、为 Vulkan 创建 Surface。
	 * 关键状态：width/height（与 framebuffer 同步）、framebufferResized（通知 Renderer 重建交换链）。
	 */
	class LVEWindow {
	public:
		LVEWindow(int width, int height, std::string windowName);
		~LVEWindow();

		// 窗口持有 GLFW 句柄，禁止拷贝，保证 RAII 唯一所有权
		LVEWindow(const LVEWindow&) = delete;
		LVEWindow& operator=(const LVEWindow&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(window); };

		/** 当前 framebuffer 像素尺寸，供交换链 extent 使用 */
		VkExtent2D getExtent() {
			return { static_cast<uint32_t>(width), static_cast<uint32_t>(height)
			}; }

		bool wasWindowResized() { return framebufferResized; }
		void resetWindowResizedFlag() { framebufferResized = false; }
		GLFWwindow* getGLFWwindow() const { return window; }

		/** 由 VkInstance 创建与本窗口关联的 VkSurfaceKHR（呈现目标） */
		void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

	private:
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
		void initWindow();

		int width;
		int height;
		bool framebufferResized = false;  // 置位后由 Renderer::endFrame 触发 recreateSwapChain

		GLFWwindow* window;
		std::string windowName;
	};
};
