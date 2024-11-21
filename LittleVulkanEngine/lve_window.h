#pragma once

#define GLFW_INCLUDE_VULKAN

#include <string>
#include <GLFW/glfw3.h>

namespace lve {
	class LVEWindow {
	public:
		LVEWindow(int width, int height, std::string windowName);
		~LVEWindow();

		//删除复制构造及复制运算符，因为窗口是指针，防止复制；资源获取即初始化，资源在窗口创建时就已经初始化完毕，清理的唯一理由是析构函数
		LVEWindow(const LVEWindow&) = delete;
		LVEWindow& operator=(const LVEWindow&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(window); };

		VkExtent2D getExtent() { 
			return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) 
			}; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

	private:
		void initWindow();
		int width;
		int height;
		
		GLFWwindow* window;
		std::string windowName;
	};
};

/*
LVEWindow(const LVEWindow&) = delete;
LVEWindow& operator=(const LVEWindow&) = delete;

这种写法的好处主要体现在以下几个方面：

防止意外复制：通过删除复制构造函数和复制赋值运算符，确保 LVEWindow 对象不能被复制。这对于管理资源（如窗口、图形上下文等）非常重要，因为复制可能会导致多个对象尝试管理同一资源，从而引发资源冲突或内存泄漏。

资源管理：在资源获取即初始化（RAII）模式下，资源在对象创建时就被分配，并在对象生命周期结束时自动释放。删除复制构造和复制运算符可以确保资源的唯一拥有权，简化资源管理。

明确对象的使用意图：这种写法清晰地表明该类的设计意图是作为一个独占的资源管理类，用户不应尝试复制它。这可以提高代码的可读性和可维护性。

避免潜在的错误：如果复制构造函数和复制赋值运算符存在，可能会导致意外的行为，比如在一个对象被复制后，两个对象都尝试释放同一资源，造成未定义行为。删除这两个函数可以避免这种错误。

提高性能：在某些情况下，复制操作可能会涉及额外的内存分配和管理，删除这些操作可以提高性能，尤其是在需要频繁创建和销毁对象的场景中。
*/