#pragma once

#include "lve_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace lve{
	class LVEModel {
	public:
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> gettAttributeDescriptions();
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
		};
		LVEModel(LVEDevice& device, const LVEModel::Builder& builder);
		~LVEModel();

		LVEModel(const LVEModel&) = delete;
		LVEModel& operator=(const LVEModel&) = delete;
		
		//LVEModel(LVEModel&&) = delete;
		//LVEModel& operator=(LVEModel&&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		LVEDevice& lveDevice;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;
	};
}


/*
1. **`#define GLM_FORCE_RADIANS`**
   ���д���ǿ�� GLM �����Ǻ��������� `sin`��`cos` �ȣ��ͽǶ���صĲ�����ʹ�û����ƣ�������Ĭ�ϵĽǶ��ơ�������Ϊ��ͼ�α���У��ܶ�������漰���Ƕȵļ���ʹ�û��ȸ�Ϊ�������� OpenGL ����ѧ�����׼���Ǻϡ�

2. **`#define GLM_FORCE_DEPTH_ZERO_TO_ONE`**
   ���д���ָ�� GLM ʹ����ȷ�Χ���㵽һ�ı�׼��������Ĭ�ϵĴӸ�ֵ����ֵ����ȷ�Χ����ͨ������ĳЩ�ִ���ͼ�� API �У��������ֵ����������Ļ�������Ȼ������Ĵ��淽ʽ����ݡ�
*/