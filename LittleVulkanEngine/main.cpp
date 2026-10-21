#include "first_app.h"

//std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

// 入口：构造 FirstApp（内部完成 Window/Device/Renderer 等）并进入主循环
int main() {
    lve::FirstApp app{};

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
