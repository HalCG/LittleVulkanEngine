#include "first_app.h"

#include "lve_camera.h"
#include "simple_render_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <array>

namespace lve {

	FirstApp::FirstApp() {
		loadGameObjects();
	}

	FirstApp::~FirstApp() {
	}

	void FirstApp::run() {
		SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass() };
		LVECamera camera{};

		while (!lveWindow.shouldClose()) {
			glfwPollEvents();

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

	std::unique_ptr<LVEModel>  FirstApp::createCubeModel(LVEDevice& device, glm::vec3 offset)
	{
		std::vector<LVEModel::Vertex> vertices{
		 // left face (white)
		 {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
		 {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
		 {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
		 {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
		 {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
		 {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
		 // right face (yellow)
		 {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
		 {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
		 {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
		 {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
		 {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
		 {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
		 // top face (orange, remember y axis points down)
		 {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
		 {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
		 {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
		 {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
		 {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
		 {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
		 // bottom face (red)
		 {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
		 {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
		 {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
		 {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
		 {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
		 {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
		 // nose face (blue)
		 {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
		 {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		 {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		 {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
		 {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
		 {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		 // tail face (green)
		 {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
		 {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		 {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		 {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
		 {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
		 {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		};
		for (auto& v : vertices) {
			v.position += offset;
		}
		return std::make_unique<LVEModel>(device, vertices);
	}

	void FirstApp::loadGameObjects() {
		std::shared_ptr<LVEModel> lveModel = createCubeModel(lveDevice, { .0f, .0f, .0f });
		auto cube = LVEGameObject::createGameObject();
		cube.model = lveModel;
		cube.transform.translation = { .0f, .0f, .5f };
		//cube.transform.scale = { .5f, .5f, .5f };//正交测试
		cube.transform.translation = { .0f, .0f, 2.5f };//透视测试
		gameObjects.push_back(std::move(cube));
	}
};