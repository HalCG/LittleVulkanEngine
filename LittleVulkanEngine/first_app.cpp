#include "first_app.h"

#include "keyboard_movement_controller.h"
#include "lve_buffer.h"
#include "lve_camera.h"
#include "point_light_system.h"
#include "simple_render_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp> // rotate

//std
#include <stdexcept>
#include <array>
#include <chrono>
#include <memory>

namespace lve {

	FirstApp::FirstApp() {
		// 池：最多分配 MAX_FRAMES_IN_FLIGHT 个 set；其中 UNIFORM_BUFFER 类型额度同样为帧数
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
		// 每帧槽一个 UBO：HOST_VISIBLE 以便 CPU 每帧 write；instanceCount=1
		std::vector<std::unique_ptr<LVEBuffer>> uboBuffers(LVESwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); ++i) {
			uboBuffers[i] = std::make_unique<LVEBuffer>(
				lveDevice,
				sizeof(GlobalUbo),                 // instanceSize
				1,                                 // instanceCount
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				);
			uboBuffers[i]->map();  // 持久 map，循环内只 write+flush
		}

		// 布局：binding 0 = UBO，vert/frag 都能读（对应 GLSL set=0,binding=0）
		auto globalSetLayout =
			LVEDescriptorSetLayout::Builder(lveDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		// 每帧一个 set，指向各自的 uboBuffers[i]，避免 CPU 写 A 时 GPU 还在读 A
		std::vector<VkDescriptorSet> globalDescriptorSets(LVESwapChain::MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();  // 填 VkDescriptorBufferInfo
			LVEDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)   // 写入 binding 0
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{
			lveDevice,
			lveRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout() };

		PointLightSystem pointLightSystem{
							lveDevice,
							lveRenderer.getSwapChainRenderPass(),
							globalSetLayout->getDescriptorSetLayout() };

		LVECamera camera{};

		// viewerObject 只驱动相机，不参与网格绘制
		auto viewerObject = LVEGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
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
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

			if (auto commandBuffer = lveRenderer.beginFrame()) {
				int frameIndex = lveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects };

				// --- 更新全局 UBO（投影/视图/点光）---
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				ubo.inverseView = camera.getInverseView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();  // 非 COHERENT 时必须 flush，否则 GPU 可能读到旧数据

				// --- 同一 RenderPass 内两条管线：先网格，再点光 Billboard ---
				lveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
				lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();  // submit + present
			}
		}

		vkDeviceWaitIdle(lveDevice.device());
	}

	void FirstApp::loadGameObjects() {
		// Models from https://github.com/blurrypiano/littleVulkanEngine/tree/main/models
		// Working directory should be $(ProjectDir) so "models/..." resolves.
		std::shared_ptr<LVEModel> lveModel =
			LVEModel::createModelFromFile(lveDevice, "models/flat_vase.obj");
		auto flatVase = LVEGameObject::createGameObject();
		flatVase.model = lveModel;
		flatVase.transform.translation = { -.5f, .5f, 0.f };
		flatVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.emplace(flatVase.getId(), std::move(flatVase));

		lveModel = LVEModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
		auto smoothVase = LVEGameObject::createGameObject();
		smoothVase.model = lveModel;
		smoothVase.transform.translation = { .5f, .5f, 0.f };
		smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		lveModel = LVEModel::createModelFromFile(lveDevice, "models/quad.obj");
		auto floor = LVEGameObject::createGameObject();
		floor.model = lveModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = { 3.f, 1.f, 3.f };
		gameObjects.emplace(floor.getId(), std::move(floor));

		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = LVEGameObject::makePointLight(0.2f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
	}
};
