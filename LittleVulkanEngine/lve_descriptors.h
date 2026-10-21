#pragma once

#include "lve_device.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace lve {

	/**
	 * 描述符集布局：规定 set 内各 binding 的类型与可访问的 shader 阶段。
	 * 对应着色器 layout(set=?, binding=?)。
	 */
	class LVEDescriptorSetLayout {
	public:
		class Builder {
		public:
			Builder(LVEDevice& lveDevice) : lveDevice{ lveDevice } {}

			Builder& addBinding(
				uint32_t binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				uint32_t count = 1);

			std::unique_ptr<LVEDescriptorSetLayout> build() const;

		private:
			LVEDevice& lveDevice;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		LVEDescriptorSetLayout(
			LVEDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~LVEDescriptorSetLayout();

		LVEDescriptorSetLayout(const LVEDescriptorSetLayout&) = delete;
		LVEDescriptorSetLayout& operator=(const LVEDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		LVEDevice& lveDevice;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		friend class LVEDescriptorWriter;
	};

	/**
	 * 描述符池：按类型预留额度，从中分配 VkDescriptorSet。
	 * maxSets / poolSizes 在创建时固定，不够时需更大池或重置。
	 */
	class LVEDescriptorPool {
	public:
		class Builder {
		public:
			Builder(LVEDevice& lveDevice) : lveDevice{ lveDevice } {}

			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t count);
			std::unique_ptr<LVEDescriptorPool> build() const;

		private:
			LVEDevice& lveDevice;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		LVEDescriptorPool(
			LVEDevice& lveDevice,
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		~LVEDescriptorPool();

		LVEDescriptorPool(const LVEDescriptorPool&) = delete;
		LVEDescriptorPool& operator=(const LVEDescriptorPool&) = delete;

		bool allocateDescriptor(
			const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
		void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
		void resetPool();

	private:
		LVEDevice& lveDevice;
		VkDescriptorPool descriptorPool;

		friend class LVEDescriptorWriter;
	};

	/**
	 * 描述符写入器：按 layout 填 WriteDescriptorSet，allocate + update。
	 * 典型链：Writer(layout, pool).writeBuffer(0, &info).build(set);
	 */
	class LVEDescriptorWriter {
	public:
		LVEDescriptorWriter(LVEDescriptorSetLayout& setLayout, LVEDescriptorPool& pool);

		LVEDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		LVEDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

		bool build(VkDescriptorSet& set);
		void overwrite(VkDescriptorSet& set);

	private:
		LVEDescriptorSetLayout& setLayout;
		LVEDescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};

}  // namespace lve
