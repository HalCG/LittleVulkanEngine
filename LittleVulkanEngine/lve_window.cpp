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

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//��һ���������� GLFW ���ڵ���ʾ����ʾ����һ�������κ� OpenGL �� OpenGL ES ������������Ĵ��ڡ�
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		/*
			nullptr: ���ĸ�����������ָ��һ�����е������ģ�����еĻ���������������Ϊ nullptr ��ʾ��ʹ�����������ġ�
			nullptr: ���������������ָ��һ������������ģ�����Ҳ����Ϊ nullptr��
		*/
		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
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