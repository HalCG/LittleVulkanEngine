#include "first_app.h"

#include "keyboard_movement_controller.h"
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

namespace lve {

	FirstApp::FirstApp() {
		loadGameObjects();
	}

	FirstApp::~FirstApp() {
	}

	void FirstApp::run() {
		SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass() };
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
				lveRenderer.beginSwapChainRenderPass(commandBuffer);
				//simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);//未进行投影变换
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);

				lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(lveDevice.device());
	}

	void FirstApp::loadGameObjects() {
#if 0
		std::shared_ptr<LVEModel> lveModel = createCubeModel(lveDevice, { .0f, .0f, .0f });
		auto cube = LVEGameObject::createGameObject();
		cube.model = lveModel;
		cube.transform.translation = { .0f, .0f, .5f };
		//cube.transform.scale = { .5f, .5f, .5f };//正交测试
		cube.transform.translation = { .0f, .0f, 2.5f };//透视测试
		gameObjects.push_back(std::move(cube));
#endif

		std::shared_ptr<LVEModel> lveModel =
			LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/flat_vase.obj");
		auto gameObj = LVEGameObject::createGameObject();
		gameObj.model = lveModel;
		gameObj.transform.translation = { .0f, .0f, 2.5f };
		gameObj.transform.scale = glm::vec3(3.f);
		gameObjects.push_back(std::move(gameObj));
	}
};