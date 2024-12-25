#include "point_light_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp> // 包含 rotate 函数

// std
#include <array>
#include <cassert>
#include <map>
#include <stdexcept>

namespace lve {

	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	PointLightSystem::PointLightSystem(
		LVEDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: lveDevice{ device } {
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	PointLightSystem::~PointLightSystem() {
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void PointLightSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		//设置渲染通道和管道布局，并清除属性描述和绑定描述（可能是为了自定义）。
		PipelineConfigInfo pipelineConfig{};
		LVEPipeline::defaultPipelineConfigInfo(pipelineConfig);
		LVEPipeline::enableAlphaBlending(pipelineConfig);
		pipelineConfig.attributeDescriptions.clear(); 
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LVEPipeline>(
			lveDevice,
			"E:/opengl/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/point_light.vert.spv",
			"E:/opengl/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/point_light.frag.spv",
			pipelineConfig);
	}

	void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, { 0.f, -1.f, 0.f });//引入帧时间，实现点光源动态旋转（0.1f 减速）
		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

			// 更新光源位置
			obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

			// ubo赋值
			ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	//执行渲染操作
	void PointLightSystem::render(FrameInfo& frameInfo) {
		// 排序光源
		std::map<float, LVEGameObject::id_t> sorted;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			// 计算距离
			auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
			float disSquared = glm::dot(offset, offset);
			sorted[disSquared] = obj.getId();
		}

		//绑定管道到命令缓冲区。
		lvePipeline->bind(frameInfo.commandBuffer);

		//绑定描述集，确保着色器可以访问必要的资源。
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);

		//更新推送常量后，draw，因为多个点光源，每个点光源的颜色、位置、大小都同
		//for (auto& kv : frameInfo.gameObjects) {
		//	auto& obj = kv.second;
		//	if (obj.pointLight == nullptr) continue;
		
		// 以相反的顺序迭代排序的灯
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
			auto& obj = frameInfo.gameObjects.at(it->second);
			PointLightPushConstants push{};
			push.position = glm::vec4(obj.transform.translation, 1.f);
			push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			push.radius = obj.transform.scale.x;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}

}  // namespace lve



/*
在 Vulkan 中，描述符集（Descriptor Set）和管道布局（Pipeline Layout）是两个重要的概念，它们在图形渲染过程中各自承担着不同的角色。下面将对它们的绑定功能、形式及优劣进行分析。

### 描述符布局（Descriptor Set Layout）

#### 功能
- 描述符布局定义了一个描述符集中包含的资源类型和数量（如纹理、缓冲区等）。
- 它为着色器提供了访问资源的方式，描述符集中的每个描述符都对应着着色器中定义的资源。

#### 形式
- 描述符布局是通过 `VkDescriptorSetLayoutCreateInfo` 结构体创建的，定义了描述符的数量、类型和阶段（如顶点着色器、片段着色器等）。
- 描述符集可以在多个管道之间共享。

#### 优劣
- **优点**：
  - 灵活性高：可以在不同的渲染通道中使用相同的描述符布局。
  - 资源复用：多个管道可以使用相同的描述符集，减少内存使用。
- **缺点**：
  - 描述符集的更新可能会影响性能，特别是在频繁切换描述符集时。

### 管道布局（Pipeline Layout）

#### 功能
- 管道布局是一个更高层次的概念，它定义了一个管道的整体结构，包括绑定的描述符集和推送常量范围。
- 管道布局将描述符集与管道关联，使得在渲染过程中可以使用这些资源。

#### 形式
- 管道布局通过 `VkPipelineLayoutCreateInfo` 结构体创建，包含了描述符布局的数量、指针和推送常量范围的信息。
- 管道布局通常在管道创建时指定，且在管道的整个生命周期内保持不变。

#### 优劣
- **优点**：
  - 管道布局提供了一个统一的接口，简化了渲染过程中的资源管理。
  - 可以通过推送常量快速传递小量数据，减少了对描述符集的依赖。
- **缺点**：
  - 一旦创建，管道布局的结构（如绑定的描述符集）就无法更改，因此在设计时需要考虑周全。
  - 每次创建新的管道时都需要重新指定管道布局，可能导致性能开销。

### 形式与优劣分析总结

1. **形式**：
   - 描述符布局主要关注描述符的类型和数量，提供了资源访问的结构。
   - 管道布局则是将描述符布局与管道结合，形成一个完整的渲染管道。

2. **优劣**：
   - 描述符布局的灵活性和资源复用使其适用于多种管道，但在频繁更新时可能影响性能。
   - 管道布局提供了更高层次的结构化管理，简化了资源绑定过程，但在创建时需要谨慎设计，且不支持动态修改。

### 结论
描述符布局和管道布局在 Vulkan 中各有其重要性，它们的设计和使用需要根据具体的渲染需求进行权衡。描述符布局提供了灵活的资源管理，而管道布局则确保了管道的高效运行和资源的有效利用。


在 Vulkan 中，不可以直接在管道中绑定描述符集，而是必须通过管道布局来定义和管理描述符集的绑定。这是 Vulkan API 的设计决策，以提供更高的灵活性和性能。下面详细解释原因及相关概念。

### 1. Vulkan 的设计原则

Vulkan 的设计旨在提供更高的控制权和性能，通过显式的状态管理让开发者能够完全掌握图形渲染的过程。这意味着许多操作需要在管道创建之前完成，以确保管道在运行时的效率。

### 2. 管道布局与描述符集的关系

- **管道布局**：
  - 管道布局在 Vulkan 中用于定义一组与管道关联的描述符集布局，以及可选的推送常量范围。
  - 在创建管道时，必须指定管道布局，这样 Vulkan 可以在管道执行期间知道如何访问相应的描述符集。

- **描述符集**：
  - 描述符集是具体的资源实例，存储了多个描述符（如纹理、缓冲区等）以及它们在 GPU 上的绑定信息。
  - 描述符集本身是独立于管道的，但其必须与管道布局一起使用。

### 3. 不能直接在管道中绑定描述符集的原因

- **效率考虑**：在管道创建时，Vulkan 会为该管道的状态进行优化。这包括对描述符集的映射和访问方式。将描述符集绑定的操作放在管道布局中，可以避免在每次渲染时都重复设置状态，提高性能。

- **设计一致性**：Vulkan 的设计理念是显式控制，使用管道布局为各种图形操作提供了一种一致的方式来管理资源。如果允许直接在管道中绑定描述符，可能会导致开发者在各种管道之间频繁变化绑定的资源，降低代码的可读性和一致性。

- **推送常量支持**：管道布局不仅定义了描述符集，还允许定义推送常量范围。这为紧急传输小量数据到着色器提供了一种灵活的机制，直接在管道中操作描述符集将无法利用这一特性。

### 4. 绑定的基本过程

在 Vulkan 中，描述符集的绑定通常涉及以下几个步骤：

1. **创建描述符集布局**：定义资源的类型和数量。
2. **分配描述符集**：为特定的实例分配描述符集。
3. **更新描述符集**：将实际资源（例如纹理、缓冲区）更新到描述符集中。
4. **创建管道布局**：将描述符集布局与管道进行关联。
5. **创建管道**：使用管道布局和其他相关状态创建图形管道。
6. **在命令缓冲区中绑定描述符集**：在绘制调用之前，通过命令缓冲区将描述符集绑定到当前的管道上。

### 结论

因此，在 Vulkan 中，描述符集不能直接在管道中绑定，而是必须通过管道布局进行管理。这种设计方式使 Vulkan 更加高效和灵活，同时也增强了对 GPU 资源的控制与管理。如果需要更改绑定的资源，只需更新描述符集，然后在命令缓冲区中将其绑定到管道上即可。

*/