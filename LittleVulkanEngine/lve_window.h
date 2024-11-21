#pragma once

#define GLFW_INCLUDE_VULKAN

#include <string>
#include <GLFW/glfw3.h>

namespace lve {
	class LVEWindow {
	public:
		LVEWindow(int width, int height, std::string windowName);
		~LVEWindow();

		//ɾ�����ƹ��켰�������������Ϊ������ָ�룬��ֹ���ƣ���Դ��ȡ����ʼ������Դ�ڴ��ڴ���ʱ���Ѿ���ʼ����ϣ������Ψһ��������������
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

����д���ĺô���Ҫ���������¼������棺

��ֹ���⸴�ƣ�ͨ��ɾ�����ƹ��캯���͸��Ƹ�ֵ�������ȷ�� LVEWindow �����ܱ����ơ�����ڹ�����Դ���細�ڡ�ͼ�������ĵȣ��ǳ���Ҫ����Ϊ���ƿ��ܻᵼ�¶�������Թ���ͬһ��Դ���Ӷ�������Դ��ͻ���ڴ�й©��

��Դ��������Դ��ȡ����ʼ����RAII��ģʽ�£���Դ�ڶ��󴴽�ʱ�ͱ����䣬���ڶ����������ڽ���ʱ�Զ��ͷš�ɾ�����ƹ���͸������������ȷ����Դ��Ψһӵ��Ȩ������Դ����

��ȷ�����ʹ����ͼ������д�������ر�������������ͼ����Ϊһ����ռ����Դ�����࣬�û���Ӧ���Ը��������������ߴ���Ŀɶ��ԺͿ�ά���ԡ�

����Ǳ�ڵĴ���������ƹ��캯���͸��Ƹ�ֵ��������ڣ����ܻᵼ���������Ϊ��������һ�����󱻸��ƺ��������󶼳����ͷ�ͬһ��Դ�����δ������Ϊ��ɾ���������������Ա������ִ���

������ܣ���ĳЩ����£����Ʋ������ܻ��漰������ڴ����͹���ɾ����Щ��������������ܣ�����������ҪƵ�����������ٶ���ĳ����С�
*/