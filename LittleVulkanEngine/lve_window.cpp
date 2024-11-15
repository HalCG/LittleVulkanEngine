#include "lve_window.h"

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

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//是一个用于设置 GLFW 窗口的提示，表示创建一个不与任何 OpenGL 或 OpenGL ES 上下文相关联的窗口。
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		/*
			nullptr: 第四个参数是用于指定一个现有的上下文（如果有的话），在这里设置为 nullptr 表示不使用现有上下文。
			nullptr: 第五个参数是用于指定一个共享的上下文，这里也设置为 nullptr。
		*/
		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	}
};