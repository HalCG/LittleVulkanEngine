#pragma once

#include "lve_buffer.h"
#include "lve_device.h"

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

//std
#include <memory>

namespace lve{
	class LVEModel {
	public:
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		
			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal &&
					uv == other.uv;
			}
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};
		LVEModel(LVEDevice& device, const LVEModel::Builder& builder);
		~LVEModel();

		LVEModel(const LVEModel&) = delete;
		LVEModel& operator=(const LVEModel&) = delete;
		
		//LVEModel(LVEModel&&) = delete;
		//LVEModel& operator=(LVEModel&&) = delete;

		static std::unique_ptr<LVEModel> createModelFromFile(
			LVEDevice& device, const std::string& filepath);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		LVEDevice& lveDevice;

		std::unique_ptr<LVEBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<LVEBuffer> indexBuffer;
		uint32_t indexCount;
	};
}


/*
1. **`#define GLM_FORCE_RADIANS`**
   ���д���ǿ�� GLM �����Ǻ��������� `sin`��`cos` �ȣ��ͽǶ���صĲ�����ʹ�û����ƣ�������Ĭ�ϵĽǶ��ơ�������Ϊ��ͼ�α���У��ܶ�������漰���Ƕȵļ���ʹ�û��ȸ�Ϊ�������� OpenGL ����ѧ�����׼���Ǻϡ�

2. **`#define GLM_FORCE_DEPTH_ZERO_TO_ONE`**
   ���д���ָ�� GLM ʹ����ȷ�Χ���㵽һ�ı�׼��������Ĭ�ϵĴӸ�ֵ����ֵ����ȷ�Χ����ͨ������ĳЩ�ִ���ͼ�� API �У��������ֵ����������Ļ�������Ȼ������Ĵ��淽ʽ����ݡ�
*/