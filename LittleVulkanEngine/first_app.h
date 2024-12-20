#pragma once

#include "lve_window.h"
#include "lve_model.h"
#include "lve_descriptors.h"
#include "lve_game_object.h"
#include "lve_renderer.h"

//std
#include <memory>
#include <vector>

namespace lve {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadGameObjects();
		std::unique_ptr<LVEModel> createCubeModel(LVEDevice& device, glm::vec3 offset);

		lve::LVEWindow lveWindow{ WIDTH, HEIGHT, "HelloVulkan!" };
		lve::LVEDevice lveDevice{ lveWindow };
		LVERenderer lveRenderer{lveWindow, lveDevice};

		// 注意：声明的顺序很重要
		std::unique_ptr<LVEDescriptorPool> globalPool{};
		std::vector<LVEGameObject> gameObjects;
	};
}