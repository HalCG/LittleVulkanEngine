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
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };		// w表示光强
		glm::vec3 lightPosition{ -1.f };
		alignas(16) glm::vec4 lightColor{ 1.f };				// w表示光强
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
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			//camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

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
		flatVase.transform.translation = { -.5f, .5f, 0.f };
		flatVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.push_back(std::move(flatVase));

		lveModel = LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/smooth_vase.obj");
		auto smoothVase = LVEGameObject::createGameObject();
		smoothVase.model = lveModel;
		smoothVase.transform.translation = { .5f, .5f, 0.f };
		smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.push_back(std::move(smoothVase));

		lveModel = LVEModel::createModelFromFile(lveDevice, "C:/Users/tolcf/Desktop/models/quad.obj");
		auto floor = LVEGameObject::createGameObject();
		floor.model = lveModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = { 3.f, 1.f, 3.f };
		gameObjects.push_back(std::move(floor));
	}
};

/*
在你提供的 `GlobalUbo` 结构体中，`alignas(16)` 是用来指定 `lightColor` 字段的内存对齐方式。这种对齐在图形编程中非常重要，尤其是在使用如 Vulkan 或 Direct3D 这类现代图形 API 时。以下是详细分析：

### 内存对齐的背景

1. **概念和目的**:
	- 内存对齐是指数据在内存中存储的方式，使其起始地址符合特定的对齐要求。一般来说，数据类型的大小和对齐要求会影响其在内存中的布局。
	- 对齐的主要目的在于提高访问效率，尤其是在 SIMD 指令（如 AVX、SSE）和 GPU 处理时，对齐可以减少内存访问延迟并提高性能。

2. **GPU 和 UBO**:
   - 在图形编程中，Uniform Buffer Objects (UBOs) 用于将统一数据传递给着色器。为保证数据按照特定格式正确传递，数据的对齐就显得尤为重要。
   - 对于许多 GPU 架构，通常要求特定的数据类型必须按照一定的字节对齐。例如，`glm::vec4` 通常需要在内存中以 16 字节对齐（因为它的大小是 16 字节）。

### 对 `lightColor` 使用 `alignas(16)`

1. **合规性**:
   - 使用 `alignas(16)` 保证 `lightColor` 的起始地址是 16 字节对齐的。这确保了数据在 GPU 阶段被正确读取，并避免因对齐不当导致的性能问题。

2. **结构布局**:
   - 在结构体中，非对齐数据可能导致结构体整体的对齐问题，从而影响后续字段的布局。在使用 `alignas(16)` 后，`lightColor` 确保其位置能够忽略其他小字段的对齐要求，保持整体结构的合理性。

3. **性能优化**:
   - 通过确保 `lightColor` 16 字节对齐，可以提高 GPU 处理它的性能，减少了访问时的内存读取成本。

### 总结

总之，在设计 `GlobalUbo` 结构体时使用 `alignas(16)` 是为了确保 `lightColor` 按照 16 字节对齐存储，以符合现代 GPU 的访问需求，提升性能并避免潜在的对齐问题。这对在实时渲染中的数据处理能力有着积极的影响。如果有更具体的应用场景或代码细节需讨论，欢迎提问！

*/