#pragma once

#include "lve_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace lve{
	class LVEModel {
	public:
		struct Vertex {
			glm::vec2 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> gettAttributeDescriptions();
		};

		LVEModel(LVEDevice& device, const std::vector<Vertex>& vertices);
		~LVEModel();

		LVEModel(const LVEModel&) = delete;
		LVEModel& operator=(const LVEModel&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);

		LVEDevice& lveDevice;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	};
}
