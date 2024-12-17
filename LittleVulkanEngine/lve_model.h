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
   这行代码强制 GLM 在三角函数（例如 `sin`、`cos` 等）和角度相关的操作中使用弧度制，而不是默认的角度制。这是因为在图形编程中，很多情况下涉及到角度的计算使用弧度更为常见，与 OpenGL 的数学计算标准相吻合。

2. **`#define GLM_FORCE_DEPTH_ZERO_TO_ONE`**
   该行代码指定 GLM 使用深度范围从零到一的标准，而不是默认的从负值到正值的深度范围。这通常用于某些现代的图形 API 中，这样深度值更易于与屏幕坐标和深度缓冲区的储存方式相兼容。
*/