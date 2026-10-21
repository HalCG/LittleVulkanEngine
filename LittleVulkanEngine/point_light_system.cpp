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

	// 与 point_light.vert/frag 的 push 块一致；color.w 为强度
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

		// 无顶点缓冲：清空 vertex input；开 alpha 混合画半透明灯光球
		PipelineConfigInfo pipelineConfig{};
		LVEPipeline::defaultPipelineConfigInfo(pipelineConfig);
		LVEPipeline::enableAlphaBlending(pipelineConfig);
		pipelineConfig.attributeDescriptions.clear(); 
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LVEPipeline>(
			lveDevice,
			"shaders/point_light.vert.spv",
			"shaders/point_light.frag.spv",
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

	void PointLightSystem::render(FrameInfo& frameInfo) {
		// 半透明需由远到近画：按与相机距离平方排序，再 reverse 迭代
		std::map<float, LVEGameObject::id_t> sorted;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;
			auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
			float disSquared = glm::dot(offset, offset);
			sorted[disSquared] = obj.getId();
		}

		lvePipeline->bind(frameInfo.commandBuffer);
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

		// 无 VB：vertexCount=6 → shader 用 gl_VertexIndex 取 OFFSETS[6] 拼两个三角形
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
			auto& obj = frameInfo.gameObjects.at(it->second);
			PointLightPushConstants push{};
			push.position = glm::vec4(obj.transform.translation, 1.f);
			push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
			push.radius = obj.transform.scale.x;  // makePointLight 把半径存在 scale.x

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);  // vertexCount, instanceCount, firstVertex, firstInstance
		}
	}

}  // namespace lve
