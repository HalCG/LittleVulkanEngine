#include "point_light_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp> // ���� rotate ����

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

		//������Ⱦͨ���͹ܵ����֣���������������Ͱ�������������Ϊ���Զ��壩��
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
		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, { 0.f, -1.f, 0.f });//����֡ʱ�䣬ʵ�ֵ��Դ��̬��ת��0.1f ���٣�
		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

			// ���¹�Դλ��
			obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

			// ubo��ֵ
			ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	//ִ����Ⱦ����
	void PointLightSystem::render(FrameInfo& frameInfo) {
		// �����Դ
		std::map<float, LVEGameObject::id_t> sorted;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.pointLight == nullptr) continue;

			// �������
			auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
			float disSquared = glm::dot(offset, offset);
			sorted[disSquared] = obj.getId();
		}

		//�󶨹ܵ������������
		lvePipeline->bind(frameInfo.commandBuffer);

		//����������ȷ����ɫ�����Է��ʱ�Ҫ����Դ��
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

		//�������ͳ�����draw����Ϊ������Դ��ÿ�����Դ����ɫ��λ�á���С��ͬ
		//for (auto& kv : frameInfo.gameObjects) {
		//	auto& obj = kv.second;
		//	if (obj.pointLight == nullptr) continue;
		
		// ���෴��˳���������ĵ�
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
�� Vulkan �У�����������Descriptor Set���͹ܵ����֣�Pipeline Layout����������Ҫ�ĸ��������ͼ����Ⱦ�����и��Գе��Ų�ͬ�Ľ�ɫ�����潫�����ǵİ󶨹��ܡ���ʽ�����ӽ��з�����

### ���������֣�Descriptor Set Layout��

#### ����
- ���������ֶ�����һ�����������а�������Դ���ͺ��������������������ȣ���
- ��Ϊ��ɫ���ṩ�˷�����Դ�ķ�ʽ�����������е�ÿ������������Ӧ����ɫ���ж������Դ��

#### ��ʽ
- ������������ͨ�� `VkDescriptorSetLayoutCreateInfo` �ṹ�崴���ģ������������������������ͺͽ׶Σ��綥����ɫ����Ƭ����ɫ���ȣ���
- �������������ڶ���ܵ�֮�乲��

#### ����
- **�ŵ�**��
  - ����Ըߣ������ڲ�ͬ����Ⱦͨ����ʹ����ͬ�����������֡�
  - ��Դ���ã�����ܵ�����ʹ����ͬ�����������������ڴ�ʹ�á�
- **ȱ��**��
  - ���������ĸ��¿��ܻ�Ӱ�����ܣ��ر�����Ƶ���л���������ʱ��

### �ܵ����֣�Pipeline Layout��

#### ����
- �ܵ�������һ�����߲�εĸ����������һ���ܵ�������ṹ�������󶨵��������������ͳ�����Χ��
- �ܵ����ֽ�����������ܵ�������ʹ������Ⱦ�����п���ʹ����Щ��Դ��

#### ��ʽ
- �ܵ�����ͨ�� `VkPipelineLayoutCreateInfo` �ṹ�崴�������������������ֵ�������ָ������ͳ�����Χ����Ϣ��
- �ܵ�����ͨ���ڹܵ�����ʱָ�������ڹܵ����������������ڱ��ֲ��䡣

#### ����
- **�ŵ�**��
  - �ܵ������ṩ��һ��ͳһ�Ľӿڣ�������Ⱦ�����е���Դ����
  - ����ͨ�����ͳ������ٴ���С�����ݣ������˶�����������������
- **ȱ��**��
  - һ���������ܵ����ֵĽṹ����󶨵��������������޷����ģ���������ʱ��Ҫ������ȫ��
  - ÿ�δ����µĹܵ�ʱ����Ҫ����ָ���ܵ����֣����ܵ������ܿ�����

### ��ʽ�����ӷ����ܽ�

1. **��ʽ**��
   - ������������Ҫ��ע�����������ͺ��������ṩ����Դ���ʵĽṹ��
   - �ܵ��������ǽ�������������ܵ���ϣ��γ�һ����������Ⱦ�ܵ���

2. **����**��
   - ���������ֵ�����Ժ���Դ����ʹ�������ڶ��ֹܵ�������Ƶ������ʱ����Ӱ�����ܡ�
   - �ܵ������ṩ�˸��߲�εĽṹ������������Դ�󶨹��̣����ڴ���ʱ��Ҫ������ƣ��Ҳ�֧�ֶ�̬�޸ġ�

### ����
���������ֺ͹ܵ������� Vulkan �и�������Ҫ�ԣ����ǵ���ƺ�ʹ����Ҫ���ݾ������Ⱦ�������Ȩ�⡣�����������ṩ��������Դ�������ܵ�������ȷ���˹ܵ��ĸ�Ч���к���Դ����Ч���á�


�� Vulkan �У�������ֱ���ڹܵ��а��������������Ǳ���ͨ���ܵ�����������͹������������İ󶨡����� Vulkan API ����ƾ��ߣ����ṩ���ߵ�����Ժ����ܡ�������ϸ����ԭ����ظ��

### 1. Vulkan �����ԭ��

Vulkan �����ּ���ṩ���ߵĿ���Ȩ�����ܣ�ͨ����ʽ��״̬�����ÿ������ܹ���ȫ����ͼ����Ⱦ�Ĺ��̡�����ζ����������Ҫ�ڹܵ�����֮ǰ��ɣ���ȷ���ܵ�������ʱ��Ч�ʡ�

### 2. �ܵ����������������Ĺ�ϵ

- **�ܵ�����**��
  - �ܵ������� Vulkan �����ڶ���һ����ܵ������������������֣��Լ���ѡ�����ͳ�����Χ��
  - �ڴ����ܵ�ʱ������ָ���ܵ����֣����� Vulkan �����ڹܵ�ִ���ڼ�֪����η�����Ӧ������������

- **��������**��
  - ���������Ǿ������Դʵ�����洢�˶�����������������������ȣ��Լ������� GPU �ϵİ���Ϣ��
  - �������������Ƕ����ڹܵ��ģ����������ܵ�����һ��ʹ�á�

### 3. ����ֱ���ڹܵ��а�����������ԭ��

- **Ч�ʿ���**���ڹܵ�����ʱ��Vulkan ��Ϊ�ùܵ���״̬�����Ż��������������������ӳ��ͷ��ʷ�ʽ�������������󶨵Ĳ������ڹܵ������У����Ա�����ÿ����Ⱦʱ���ظ�����״̬��������ܡ�

- **���һ����**��Vulkan �������������ʽ���ƣ�ʹ�ùܵ�����Ϊ����ͼ�β����ṩ��һ��һ�µķ�ʽ��������Դ���������ֱ���ڹܵ��а������������ܻᵼ�¿������ڸ��ֹܵ�֮��Ƶ���仯�󶨵���Դ�����ʹ���Ŀɶ��Ժ�һ���ԡ�

- **���ͳ���֧��**���ܵ����ֲ������������������������������ͳ�����Χ����Ϊ��������С�����ݵ���ɫ���ṩ��һ�����Ļ��ƣ�ֱ���ڹܵ��в��������������޷�������һ���ԡ�

### 4. �󶨵Ļ�������

�� Vulkan �У����������İ�ͨ���漰���¼������裺

1. **����������������**��������Դ�����ͺ�������
2. **������������**��Ϊ�ض���ʵ����������������
3. **������������**����ʵ����Դ���������������������µ����������С�
4. **�����ܵ�����**������������������ܵ����й�����
5. **�����ܵ�**��ʹ�ùܵ����ֺ��������״̬����ͼ�ιܵ���
6. **����������а���������**���ڻ��Ƶ���֮ǰ��ͨ��������������������󶨵���ǰ�Ĺܵ��ϡ�

### ����

��ˣ��� Vulkan �У�������������ֱ���ڹܵ��а󶨣����Ǳ���ͨ���ܵ����ֽ��й���������Ʒ�ʽʹ Vulkan ���Ӹ�Ч����ͬʱҲ��ǿ�˶� GPU ��Դ�Ŀ�������������Ҫ���İ󶨵���Դ��ֻ���������������Ȼ������������н���󶨵��ܵ��ϼ��ɡ�

*/