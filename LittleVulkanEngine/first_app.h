#pragma once

#include "lve_window.h"

namespace lve {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		void run();


	private:
		lve::LVEWindow lveWindow{ WIDTH, HEIGHT, "HelloVulkan!" };
	};
}