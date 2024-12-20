#pragma once

#include "lve_device.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

//ʵ���� Vulkan �����������֣�LVEDescriptorSetLayout������������ (LVEDescriptorPool) �Լ�������д������LVEDescriptorWriter����
namespace lve {

	//LVEDescriptorSetLayout ����Ҫ���ڹ��� Vulkan �������������֣������������ֶ�����һ�����������еĹ̶����ֺ����͡�����������������ɫ�������з��ʻ�������ͼ����Դ��
	class LVEDescriptorSetLayout {
	public:
		//ʹ�ý�����ģʽ������ LVEDescriptorSetLayout ����
		class Builder {
		public:
			Builder(LVEDevice& lveDevice) : lveDevice{ lveDevice } {}

			//������Ӱ���Ϣ�����������������������͡�ʹ�õ� shader �׶κͰ�������
			Builder& addBinding(
				uint32_t binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				uint32_t count = 1);

			//�������յ� LVEDescriptorSetLayout��
			std::unique_ptr<LVEDescriptorSetLayout> build() const;

		private:
			LVEDevice& lveDevice;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		//LVEDescriptorPool �����ڹ��� Vulkan �������أ�������������������һ�����������ġ�
		LVEDescriptorSetLayout(
			LVEDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~LVEDescriptorSetLayout();

		//���ÿ�������͸�ֵ,���ⲻ��Ҫ����Դ���������� GPU ��Դ�������ԣ�ȷ����Դ�İ�ȫʹ�á�
		LVEDescriptorSetLayout(const LVEDescriptorSetLayout&) = delete;
		LVEDescriptorSetLayout& operator=(const LVEDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		LVEDevice& lveDevice;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		friend class LVEDescriptorWriter;
	};

	//LVEDescriptorPool �����ڹ��� Vulkan �������أ�������������������һ�����������ġ�
	class LVEDescriptorPool {
	public:
		class Builder {
		public:
			Builder(LVEDevice& lveDevice) : lveDevice{ lveDevice } {}

			//������������ͺ�������
			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			//���óصı�־��
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			//���������������������
			Builder& setMaxSets(uint32_t count);
			//�������յ� LVEDescriptorPool��
			std::unique_ptr<LVEDescriptorPool> build() const;

		private:
			LVEDevice& lveDevice;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		//��ʼ���������صĴ�С����־�ͳش�С��
		LVEDescriptorPool(
			LVEDevice& lveDevice,
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		~LVEDescriptorPool();

		//���ÿ�������͸�ֵ,���ⲻ��Ҫ����Դ���������� GPU ��Դ�������ԣ�ȷ����Դ�İ�ȫʹ�á�
		LVEDescriptorPool(const LVEDescriptorPool&) = delete;
		LVEDescriptorPool& operator=(const LVEDescriptorPool&) = delete;

		//�ӳ��з���һ������������
		bool allocateDescriptor(
			const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
		//�ͷŶ������������
		void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
		//�����������أ��Ա�����ʹ�á�
		void resetPool();

	private:
		LVEDevice& lveDevice;
		VkDescriptorPool descriptorPool;

		friend class LVEDescriptorWriter;
	};

	//LVEDescriptorWriter �����ڼ򻯶�����������д����������������������ֺ��������صĹ���������һ��
	class LVEDescriptorWriter {
	public:
		LVEDescriptorWriter(LVEDescriptorSetLayout& setLayout, LVEDescriptorPool& pool);

		//����������������д�뻺������Ϣ��
		LVEDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		//����������������д��ͼ����Ϣ��
		LVEDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

		//����ʵ�ʵ�����������
		bool build(VkDescriptorSet& set);
		//��д�Ѵ��ڵ�����������
		void overwrite(VkDescriptorSet& set);

	private:
		LVEDescriptorSetLayout& setLayout;
		LVEDescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};

}  // namespace lve
