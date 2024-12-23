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

	//������һ���������ͳ�����Χ�Ĺ��߲��֣��Ա�����ɫ��֮�䴫�ݶ�̬���ݡ�
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

	//���ò�����ͼ����Ⱦ�������Ⱦ�ܵ�������ָ����ɫ���ļ�·�����������á�
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

		//��һ����Ϊ globalDescriptorSet ���������󶨵�ͼ�ι��ߣ��Ա��ں����Ļ��Ƶ����У���ɫ���ܹ��������ж������Դ��
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,			//����ʾ���ǽ��ڸ���������а���������
			VK_PIPELINE_BIND_POINT_GRAPHICS,	//��ʾ�������ڰ���������ͼ�ι��ߣ������ڻ��Ʋ����Ĺ��ߣ���
			pipelineLayout,						//���߲��ֶ����˹������������Ƶ������Ľṹ����ָ������ͼ�ι�����Ⱦʱ�������İ��ŷ�ʽ��ȷ�� GPU ֪������Щ�������ж�ȡ���ݡ�
			0,									//�������Ķ�̬ƫ������������� 0����ʾû��ʹ�ö�̬ƫ��
			1,									//Ҫ�󶨵���������������
			&frameInfo.globalDescriptorSet,		//ָ����������ָ�룬���ڰ󶨵���ǰ���������������� globalDescriptorSet ͨ����������ɫ���е�ȫ����Դ������ʡ���Դ�ȣ���ص���������
			0,									//��̬ƫ������Ϊ 0����ʾ����û�ж���Ķ�̬ƫ����ҪӦ�á�
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
