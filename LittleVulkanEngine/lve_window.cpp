#include "lve_window.h"
#include <stdexcept>

namespace lve {
	LVEWindow::LVEWindow(int width, int height, std::string windowName) :width{ width }, height{ height }, windowName{ windowName }{
		this->initWindow();
	}

	LVEWindow::~LVEWindow(){
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void LVEWindow::initWindow() {
		glfwInit();

		// 不创建 OpenGL 上下文，仅要原生窗口供 Vulkan 使用
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		// 把 this 存进 GLFW，供静态回调取回 LVEWindow*
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void LVEWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		
		}
	}

	void LVEWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto lveWindow = reinterpret_cast<LVEWindow*>(glfwGetWindowUserPointer(window));
		lveWindow->framebufferResized = true;
		lveWindow->width = width;
		lveWindow->height = height;
	}
};
