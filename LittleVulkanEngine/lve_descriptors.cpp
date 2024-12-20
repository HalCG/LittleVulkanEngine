#include "lve_descriptors.h"

// std
#include <cassert>
#include <stdexcept>

namespace lve {
	//��������Щ�������ǵ�Ч�ģ�Ӧ�ø������ǵİ�Ƶ�ʷ���Ϊ����
	// *************** Descriptor Set Layout Builder *********************

	//���һ���µİ���Ϣ�������������ֵĹ������У��Ա�������������������֡�
	LVEDescriptorSetLayout::Builder& LVEDescriptorSetLayout::Builder::addBinding(
		uint32_t binding,						//�󶨵�������ÿ���󶨶���һ��Ψһ���������������������������б�ʶ�ð󶨡�
		VkDescriptorType descriptorType,		//�����������͡���ָ���ð���ʹ�õ���Դ���ͣ�������ǻ�������ͼ��ȡ�
		VkShaderStageFlags stageFlags,			//��ɫ���׶α�־����Щ��־ָ����Щ��ɫ���׶ο��Է��ʵ�ǰ�󶨵���Դ�����綥����ɫ����Ƭ����ɫ���ȣ���
		uint32_t count)							//����������������ͨ���ڰ�����������ʱʹ�á�����ǵ�����Դ����ͨ��Ϊ 1��
	{
		assert(bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{};		//�洢�����������󶨵���Ϣ��
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;					//�¶���İ���Ϣ��¼���������С�
		return *this;										//���ص�ǰ��������ã���֧����ʽ���á����������������� addBinding ��������Ӷ���󶨡�
	}

	std::unique_ptr<LVEDescriptorSetLayout> LVEDescriptorSetLayout::Builder::build() const {
		return std::make_unique<LVEDescriptorSetLayout>(lveDevice, bindings);
	}

	// *************** Descriptor Set Layout *********************

	LVEDescriptorSetLayout::LVEDescriptorSetLayout(
		LVEDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
		: lveDevice{ lveDevice }, bindings{ bindings } 
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};//��ȡ�������������ְ���Ϣ��
		for (auto kv : bindings) {
			setLayoutBindings.push_back(kv.second);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(
			lveDevice.device(),
			&descriptorSetLayoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	LVEDescriptorSetLayout::~LVEDescriptorSetLayout() {
		vkDestroyDescriptorSetLayout(lveDevice.device(), descriptorSetLayout, nullptr);
	}

	// *************** Descriptor Pool Builder *********************
	//���������������һ�������������Լ���������count�����з���ĸ�����������������
	LVEDescriptorPool::Builder& LVEDescriptorPool::Builder::addPoolSize(
		VkDescriptorType descriptorType, uint32_t count) 
	{
		poolSizes.push_back({ descriptorType, count });
		return *this;
	}

	//�������صĴ�����־�����Ƴص���Ϊ�����磬�Ƿ�������õȣ���
	LVEDescriptorPool::Builder& LVEDescriptorPool::Builder::setPoolFlags(
		VkDescriptorPoolCreateFlags flags) {
		poolFlags = flags;
		return *this;
	}

	//�����������ؿ��Է���������������������
	LVEDescriptorPool::Builder& LVEDescriptorPool::Builder::setMaxSets(uint32_t count) {
		maxSets = count;
		return *this;
	}

	std::unique_ptr<LVEDescriptorPool> LVEDescriptorPool::Builder::build() const {
		return std::make_unique<LVEDescriptorPool>(lveDevice, maxSets, poolFlags, poolSizes);
	}

	// *************** Descriptor Pool *********************
	//����һ�� Vulkan �������أ������豸����󼯺�����������־���������ش�С��Ϣ����ʹ����Щ��Ϣ�������������صĲ�����
	LVEDescriptorPool::LVEDescriptorPool(
		LVEDevice& lveDevice,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize>& poolSizes)
		: lveDevice{ lveDevice } 
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(lveDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	LVEDescriptorPool::~LVEDescriptorPool() {
		vkDestroyDescriptorPool(lveDevice.device(), descriptorPool, nullptr);
	}

	//���ڴ����������з���һ������������
	bool LVEDescriptorPool::allocateDescriptor(
		const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;				//��ʾ�����ĸ����������з�������������
		allocInfo.pSetLayouts = &descriptorSetLayout;			//ָ����������������Ĳ��֡�
		allocInfo.descriptorSetCount = 1;						//��ʾҪ����һ������������

		// ������Ҫ����һ����DescriptorPoolManager�������������������������
		// ÿ���ɳ�����ʱ�ʹ����³ء����ⳬ��������Ŀǰ�ķ�Χ
		if (vkAllocateDescriptorSets(lveDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
			return false;
		}
		return true;
	}

	void LVEDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
		vkFreeDescriptorSets(
			lveDevice.device(),
			descriptorPool,
			static_cast<uint32_t>(descriptors.size()),
			descriptors.data());
	}

	void LVEDescriptorPool::resetPool() {
		vkResetDescriptorPool(lveDevice.device(), descriptorPool, 0);
	}

	// *************** Descriptor Writer *********************

	LVEDescriptorWriter::LVEDescriptorWriter(LVEDescriptorSetLayout& setLayout, LVEDescriptorPool& pool)
		: setLayout{ setLayout }, pool{ pool } {}

	LVEDescriptorWriter& LVEDescriptorWriter::writeBuffer(
		uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;				//ָ��Ҫд������������ľ���λ�á�
		write.pBufferInfo = bufferInfo;			//д�������������֪����һ������������Ϣ��Ҫ���¡������йػ���������Ϣ���绺�����ľ����ƫ�����ͷ�Χ��
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	//��ͼ����Ϣд�� Vulkan ���������С�
	//Ϊָ�����������󶨴���һ��д������������������ע��һ��ͼ�����Ϣ����ͼ�����ͼ�Ͳ�������Ϣ��������д����Ϣ�洢��һ��д�����б��У��Ա������������������
	LVEDescriptorWriter& LVEDescriptorWriter::writeImage(
		uint32_t binding, VkDescriptorImageInfo* imageInfo) {
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding; 
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	bool LVEDescriptorWriter::build(VkDescriptorSet& set) {
		bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
		if (!success) {
			return false;
		}
		overwrite(set);
		return true;
	}

	void LVEDescriptorWriter::overwrite(VkDescriptorSet& set) {
		for (auto& write : writes) {
			write.dstSet = set;
		}
		vkUpdateDescriptorSets(pool.lveDevice.device(), writes.size(), writes.data(), 0, nullptr);
	}

}  // namespace lve


/*
�� Vulkan �У�**��������**��**��������**��**������������**��**��������д��**֮������Ž��ܵĹ�ϵ�����ǹ�ͬ������ Vulkan ��Դ�������ɫ����Դ�󶨵Ļ����������Ƕ�ÿ��Ҫ�ص���ϸ���������໥��ϵ��

### 1. ����������Descriptor Set��
����������һ�����ϣ�������һ����������Descriptor������Щ������ָ���� GPU ��Դ���������������ȣ���������������Ⱦ�����������ṩ��ɫ���������Դ���ݡ�ÿ�������������԰�������������������ڻ��Ƶ����лὫ��󶨵�ͼ�ι��ߡ�

### 2. �������أ�Descriptor Pool��
���������� Vulkan �������������������ڴ�Ľṹ�����������Ƕ�����Է�������������������͡����������ڴ���ʱ��Ҫָ�����С�������ɵ����������͡�ʹ���������ؿ��Է���ع������������������

### 3. �����������֣�Descriptor Set Layout��
�����������ֶ��������������Ľṹ������������������ͺͰ���Ϣ���������ˣ����������������������е�λ�ã��Լ���Щ�����������ͣ���ͳһ��������ͼ��������洢�������ȣ�����󶨵�����ɫ���е�ʹ�÷�ʽ���������������ǹ̶��ģ�һ��������㲻�ܸ��ġ�

### 4. ��������д�루Descriptor Set Write��
��������д���ǽ�ʵ����Դ�����绺�����������󶨵����������ж�Ӧ������������ͨ��ͨ�� `vkUpdateDescriptorSets` ������ɣ�ͨ������������Դ��Ϣ��������䵽���������С�����Ⱦ�����У���ɫ����ʹ����Щ�����������ʰ󶨺õ���Դ��

### ��Щ���֮��Ĺ�ϵ
1. **����˳��**��
   - ���ȣ���Ҫ���������������֣��ڲ����ж��������������ͼ�������
   - ��Σ������������أ���������Ҫ�����������ͺ�����Ԥ�����ڴ档
   - Ȼ�󣬴����������з�����������������ʱ��Ҫ�ṩ֮ǰ�����������������֡�
   - ��󣬽�������Դ����Ϣд������������

2. **�໥����**��
   - �����������־��������������ĸ�ʽ��ʹ�÷�ʽ��ȷ��������������ʹ�õ����������ͺ���������󶨵���ɫ��һ�¡�
   - �����������ڷ�����������ʱʹ�õģ�ȷ�������㹻���ڴ�ɹ����䡣
   - ��������д����������������������䲼�֣�ȷ����Դ��ȷ�󶨵���������

### ��ʾ��

�����Ǵ��������������ء����ּ�д��ļ�Ҫ����ʾ����

```c
// 1. ������������
VkDescriptorSetLayoutBinding layoutBinding = {};
layoutBinding.binding = 0; // �󶨵�
layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // ����������
layoutBinding.descriptorCount = 1; // ����������
layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // ʹ�ý׶�
layoutBinding.pImmutableSamplers = nullptr; // ��ѡ������������

VkDescriptorSetLayoutCreateInfo layoutInfo = {};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &layoutBinding;

VkDescriptorSetLayout descriptorSetLayout;
vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);

// 2. ��������
VkDescriptorPoolSize poolSize = {};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // ����������
poolSize.descriptorCount = 1; // ����������

VkDescriptorPoolCreateInfo poolInfo = {};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
poolInfo.maxSets = 1; // ������һ����������

VkDescriptorPool descriptorPool;
vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

// 3. ������������
VkDescriptorSetAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = 1;
allocInfo.pSetLayouts = &descriptorSetLayout; // ʹ�õĲ���

VkDescriptorSet descriptorSet;
vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

// 4. ��������������д�룩
VkDescriptorBufferInfo bufferInfo = {};
bufferInfo.buffer = uniformBuffer; // ָ��ʵ�ʵĻ�����
bufferInfo.offset = 0;
bufferInfo.range = sizeof(UniformBufferObject); // ��������С

VkWriteDescriptorSet descriptorWrite = {};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSet; // Ŀ����������
descriptorWrite.dstBinding = 0; // ��λ��
descriptorWrite.dstArrayElement = 0;
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // ����������
descriptorWrite.descriptorCount = 1;
descriptorWrite.pBufferInfo = &bufferInfo; // ʵ�ʰ󶨵Ļ�������Ϣ

vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

### С��
- **������������**���������������Ľṹ��
- **��������**�ṩ�������������ڴ����
- **��������**ʵ�ʴ洢������ GPU ��Դ��
- **��������д��**����Դ��Ϣ�󶨵����������С�

������һͬ������ Vulkan ������Դ����ɫ����ͨ�ŵĻ���ģ�͡�����н�һ�������ʻ�����Ҫ����������ӣ�����ʱ���ң�

*/