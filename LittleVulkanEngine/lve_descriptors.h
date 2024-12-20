#pragma once

#include "lve_device.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

//实现了 Vulkan 描述符集布局（LVEDescriptorSetLayout）、描述符池 (LVEDescriptorPool) 以及描述符写入器（LVEDescriptorWriter）。
namespace lve {

	//LVEDescriptorSetLayout 类主要用于管理 Vulkan 的描述符集布局，描述符集布局定义了一个描述符集中的固定布局和类型。描述符集用于在着色器程序中访问缓冲区和图像资源。
	class LVEDescriptorSetLayout {
	public:
		//使用建造者模式来构建 LVEDescriptorSetLayout 对象。
		class Builder {
		public:
			Builder(LVEDevice& lveDevice) : lveDevice{ lveDevice } {}

			//用于添加绑定信息，包含绑定索引、描述符类型、使用的 shader 阶段和绑定数量。
			Builder& addBinding(
				uint32_t binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				uint32_t count = 1);

			//构建最终的 LVEDescriptorSetLayout。
			std::unique_ptr<LVEDescriptorSetLayout> build() const;

		private:
			LVEDevice& lveDevice;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		//LVEDescriptorPool 类用于管理 Vulkan 描述符池，描述符池是用来分配一组描述符集的。
		LVEDescriptorSetLayout(
			LVEDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~LVEDescriptorSetLayout();

		//禁用拷贝构造和赋值,避免不必要的资源拷贝。保护 GPU 资源的完整性，确保资源的安全使用。
		LVEDescriptorSetLayout(const LVEDescriptorSetLayout&) = delete;
		LVEDescriptorSetLayout& operator=(const LVEDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		LVEDevice& lveDevice;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		friend class LVEDescriptorWriter;
	};

	//LVEDescriptorPool 类用于管理 Vulkan 描述符池，描述符池是用来分配一组描述符集的。
	class LVEDescriptorPool {
	public:
		class Builder {
		public:
			Builder(LVEDevice& lveDevice) : lveDevice{ lveDevice } {}

			//添加描述符类型和数量。
			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			//设置池的标志。
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			//设置最大描述符集数量。
			Builder& setMaxSets(uint32_t count);
			//构建最终的 LVEDescriptorPool。
			std::unique_ptr<LVEDescriptorPool> build() const;

		private:
			LVEDevice& lveDevice;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		//初始化描述符池的大小、标志和池大小。
		LVEDescriptorPool(
			LVEDevice& lveDevice,
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		~LVEDescriptorPool();

		//禁用拷贝构造和赋值,避免不必要的资源拷贝。保护 GPU 资源的完整性，确保资源的安全使用。
		LVEDescriptorPool(const LVEDescriptorPool&) = delete;
		LVEDescriptorPool& operator=(const LVEDescriptorPool&) = delete;

		//从池中分配一个描述符集。
		bool allocateDescriptor(
			const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
		//释放多个描述符集。
		void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
		//重置描述符池，以便重新使用。
		void resetPool();

	private:
		LVEDevice& lveDevice;
		VkDescriptorPool descriptorPool;

		friend class LVEDescriptorWriter;
	};

	//LVEDescriptorWriter 类用于简化对描述符集的写入操作，它将描述符集布局和描述符池的功能整合在一起。
	class LVEDescriptorWriter {
	public:
		LVEDescriptorWriter(LVEDescriptorSetLayout& setLayout, LVEDescriptorPool& pool);

		//用于向描述符集中写入缓冲区信息。
		LVEDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		//用于向描述符集中写入图像信息。
		LVEDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

		//构建实际的描述符集。
		bool build(VkDescriptorSet& set);
		//重写已存在的描述符集。
		void overwrite(VkDescriptorSet& set);

	private:
		LVEDescriptorSetLayout& setLayout;
		LVEDescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};

}  // namespace lve
