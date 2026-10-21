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
	/**
	 * 应用入口类：组装窗口/设备/渲染器、描述符池、场景对象，并驱动主循环。
	 *
	 * 成员声明顺序很重要（析构逆序）：
	 * Window → Device → Renderer → globalPool → gameObjects
	 * 保证 Renderer/Device 仍可用时再销毁依赖它们的池与 GPU 资源。
	 */
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

		// 注意：声明顺序决定析构顺序；pool 必须在 device 之前析构完成相关集
		std::unique_ptr<LVEDescriptorPool> globalPool{};
		LVEGameObject::Map gameObjects;
	};
}
