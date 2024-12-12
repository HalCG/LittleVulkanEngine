#pragma once

#include "lve_window.h"
#include "lve_model.h"
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

		lve::LVEWindow lveWindow{ WIDTH, HEIGHT, "HelloVulkan!" };
		lve::LVEDevice lveDevice{ lveWindow };
		LVERenderer lveRenderer{lveWindow, lveDevice};

		std::vector<LVEGameObject> gameObjects;
	};
}