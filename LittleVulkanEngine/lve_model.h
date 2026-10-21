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
	/**
	 * GPU 网格资源。
	 * Vertex location 必须与 sample_shader.vert 的 layout(location=…) 一致。
	 * 创建时经 staging 拷到 DEVICE_LOCAL 顶点/索引缓冲。
	 */
	class LVEModel {
	public:
		struct Vertex {
			glm::vec3 position{};  // location 0
			glm::vec3 color{};     // location 1
			glm::vec3 normal{};    // location 2
			glm::vec2 uv{};        // location 3

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		
			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal &&
					uv == other.uv;
			}
		};

		/** CPU 侧装载：vertices/indices；loadModel 用 tinyobj 读 .obj */
		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};
		LVEModel(LVEDevice& device, const LVEModel::Builder& builder);
		~LVEModel();

		LVEModel(const LVEModel&) = delete;
		LVEModel& operator=(const LVEModel&) = delete;

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
