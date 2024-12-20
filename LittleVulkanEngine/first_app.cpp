#include "first_app.h"

#include "keyboard_movement_controller.h"
#include "lve_buffer.h"
#include "lve_camera.h"
#include "simple_render_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <array>
#include <chrono>
#include <memory>

namespace lve {

	struct GlobalUbo {
		glm::mat4 projectionView{ 1.f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
	};

	FirstApp::FirstApp() {
		globalPool =
			LVEDescriptorPool::Builder(lveDevice)
			.setMaxSets(LVESwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LVESwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		loadGameObjects();
	}

	FirstApp::~FirstApp() {
	}

	void FirstApp::run() {
		//LVEBuffer globalUboBuffer{
		//	  lveDevice,
		//	  sizeof(GlobalUbo),
		//	  LVESwapChain::MAX_FRAMES_IN_FLIGHT,//MAX_FRAMES_IN_FLIGHT  !!!全局使用，约定数量：frame、uboBuffer、render commandBuffers、imageAvailableSemaphores、renderFinishedSemaphores、inFlightFences
		//	  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//	  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,//未选择一致性，避免干扰正在渲染的缓冲
		//	  lveDevice.properties.limits.minUniformBufferOffsetAlignment,
		//};
		//globalUboBuffer.map();

		std::vector<std::unique_ptr<LVEBuffer>> uboBuffers(LVESwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); ++i) {
			uboBuffers[i] = std::make_unique<LVEBuffer>(
				lveDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				);
			uboBuffers[i]->map();
		}

		auto globalSetLayout =
			LVEDescriptorSetLayout::Builder(lveDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();

		//表示在多帧渲染时，我们需要为每一帧准备一个描述集。这个设定通常用于支持双缓冲或多缓冲渲染，以减少 GPU 和 CPU 之间的等待时间。
		std::vector<VkDescriptorSet> globalDescriptorSets(LVESwapChain::MAX_FRAMES_IN_FLIGHT);

		//填充描述集：需要描述符集布局、描述符池、描述符具体指向的对象（buffer、image...）
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			LVEDescriptorWriter(*globalSetLayout, *globalPool)		//创建一个描述集写入器对象利用之前创建的描述集布局和描述池。
				.writeBuffer(0, &bufferInfo)						//将第 0 号绑定位置和对应的缓冲区描述信息写入描述集中。这个缓冲区信息会将数据传递给着色器
				.build(globalDescriptorSets[i]);					//构建描述集，并将其存储到对应的
		}


		SimpleRenderSystem simpleRenderSystem{
			lveDevice,
			lveRenderer.getSwapChainRenderPass(),			//获取交换链的渲染通道，通常包含进行绘图时所需的详细信息。
			globalSetLayout->getDescriptorSetLayout() };	//取之前创建的全局描述集布局，以便在渲染过程中使用。

		LVECamera camera{};
		////camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
		//camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));

		auto viewerObject = LVEGameObject::createGameObject();
		KeyboardMovementController cameraController{};
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (!lveWindow.shouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime =
				std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;
			cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = lveRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

			if (auto commandBuffer = lveRenderer.beginFrame()) {
				int frameIndex = lveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex] };

				// update
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();//!!! 注意 当前渲染管线对应的ubo在这里刷新，被告知存储位置及长度

				// render
				lveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
				lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(lveDevice.device());
	}

	void FirstApp::loadGameObjects() {
		std::shared_ptr<LVEModel> lveModel =
			LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/flat_vase.obj");
		auto flatVase = LVEGameObject::createGameObject();
		flatVase.model = lveModel;
		flatVase.transform.translation = { -.5f, .5f, 2.5f };
		flatVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.push_back(std::move(flatVase));
		lveModel = LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/smooth_vase.obj");
		auto smoothVase = LVEGameObject::createGameObject();
		smoothVase.model = lveModel;
		smoothVase.transform.translation = { .5f, .5f, 2.5f };
		smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.push_back(std::move(smoothVase));
	}
};