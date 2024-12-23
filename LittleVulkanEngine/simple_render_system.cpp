#include "simple_render_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace lve {

	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	SimpleRenderSystem::SimpleRenderSystem(
		LVEDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: lveDevice{ device } 
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	//创建了一个包含推送常量范围的管线布局，以便在着色器之间传递动态数据。
	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

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

	//配置并创建图形渲染所需的渲染管道，包括指定着色器文件路径和其他设置。
	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		LVEPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LVEPipeline>(
			lveDevice,
			"E:/opengl/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.vert.spv",
			"E:/opengl/HalCG/LittleVulkanEngine/LittleVulkanEngine/shaders/sample_shader.frag.spv",
			pipelineConfig);
	}

	void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo)
	{
		lvePipeline->bind(frameInfo.commandBuffer);

		//将一个名为 globalDescriptorSet 的描述集绑定到图形管线，以便在后续的绘制调用中，着色器能够访问其中定义的资源。
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,			//它表示我们将在该命令缓冲区中绑定描述集。
			VK_PIPELINE_BIND_POINT_GRAPHICS,	//表示我们正在绑定描述集到图形管线（即用于绘制操作的管线）。
			pipelineLayout,						//管线布局定义了关于描述集和推导常量的结构。它指定了在图形管线渲染时描述集的安排方式，确保 GPU 知道从哪些描述集中读取数据。
			0,									//描述集的动态偏移量。传入的是 0，表示没有使用动态偏移
			1,									//要绑定的描述集的数量。
			&frameInfo.globalDescriptorSet,		//指向描述集的指针，用于绑定到当前的命令缓冲区。这里的 globalDescriptorSet 通常包含与着色器中的全局资源（如材质、光源等）相关的描述集。
			0,									//动态偏移数量为 0，表示我们没有额外的动态偏移需要应用。
			nullptr);

		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.model == nullptr) continue;
			SimplePushConstantData push{};
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}
	}
}  // namespace lve
