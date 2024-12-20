#include "lve_descriptors.h"

// std
#include <cassert>
#include <stdexcept>

namespace lve {
	//单独绑定这些描述符是低效的，应该根据它们的绑定频率分组为集合
	// *************** Descriptor Set Layout Builder *********************

	//添加一个新的绑定信息到描述符集布局的构建器中，以便后续创建描述符集布局。
	LVEDescriptorSetLayout::Builder& LVEDescriptorSetLayout::Builder::addBinding(
		uint32_t binding,						//绑定的索引。每个绑定都有一个唯一的索引，用于在描述符集布局中标识该绑定。
		VkDescriptorType descriptorType,		//描述符的类型。它指定该绑定所使用的资源类型，这可能是缓冲区、图像等。
		VkShaderStageFlags stageFlags,			//着色器阶段标志。这些标志指明哪些着色器阶段可以访问当前绑定的资源（例如顶点着色器、片段着色器等）。
		uint32_t count)							//描述符的数量。这通常在绑定是数组类型时使用。如果是单个资源，则通常为 1。
	{
		assert(bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{};		//存储关于描述符绑定的信息。
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;					//新定义的绑定信息记录到构建器中。
		return *this;										//返回当前对象的引用，以支持链式调用。这样可以连续调用 addBinding 方法来添加多个绑定。
	}

	std::unique_ptr<LVEDescriptorSetLayout> LVEDescriptorSetLayout::Builder::build() const {
		return std::make_unique<LVEDescriptorSetLayout>(lveDevice, bindings);
	}

	// *************** Descriptor Set Layout *********************

	LVEDescriptorSetLayout::LVEDescriptorSetLayout(
		LVEDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
		: lveDevice{ lveDevice }, bindings{ bindings } 
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};//提取的描述符集布局绑定信息。
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
	//向描述符池中添加一种描述符类型以及其数量。count：池中分配的该类型描述符的上限
	LVEDescriptorPool::Builder& LVEDescriptorPool::Builder::addPoolSize(
		VkDescriptorType descriptorType, uint32_t count) 
	{
		poolSizes.push_back({ descriptorType, count });
		return *this;
	}

	//描述符池的创建标志，控制池的行为（例如，是否可以重用等）。
	LVEDescriptorPool::Builder& LVEDescriptorPool::Builder::setPoolFlags(
		VkDescriptorPoolCreateFlags flags) {
		poolFlags = flags;
		return *this;
	}

	//设置描述符池可以分配的最大描述符集数量。
	LVEDescriptorPool::Builder& LVEDescriptorPool::Builder::setMaxSets(uint32_t count) {
		maxSets = count;
		return *this;
	}

	std::unique_ptr<LVEDescriptorPool> LVEDescriptorPool::Builder::build() const {
		return std::make_unique<LVEDescriptorPool>(lveDevice, maxSets, poolFlags, poolSizes);
	}

	// *************** Descriptor Pool *********************
	//创建一个 Vulkan 描述符池，接收设备、最大集合数、创建标志和描述符池大小信息，并使用这些信息来设置描述符池的参数。
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

	//用于从描述符池中分配一个描述符集。
	bool LVEDescriptorPool::allocateDescriptor(
		const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;				//表示将从哪个描述符池中分配描述符集。
		allocInfo.pSetLayouts = &descriptorSetLayout;			//指定分配的描述符集的布局。
		allocInfo.descriptorSetCount = 1;						//表示要分配一个描述符集。

		// 可能想要创建一个“DescriptorPoolManager”类来处理这种情况，并构建
		// 每当旧池填满时就创建新池。但这超出了我们目前的范围
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
		write.dstBinding = binding;				//指明要写入的描述符集的具体位置。
		write.pBufferInfo = bufferInfo;			//写入的描述符集就知道哪一个缓冲区的信息需要更新。包含有关缓冲区的信息，如缓冲区的句柄、偏移量和范围。
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	//将图像信息写入 Vulkan 描述符集中。
	//为指定的描述符绑定创建一个写操作，向描述符集中注册一个图像的信息（如图像的视图和采样器信息），并将写入信息存储在一个写操作列表中，以便后续更新描述符集。
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
在 Vulkan 中，**描述符集**、**描述符池**、**描述符集布局**和**描述符集写入**之间存在着紧密的关系，它们共同构成了 Vulkan 资源管理和着色器资源绑定的基础。以下是对每个要素的详细描述及其相互关系。

### 1. 描述符集（Descriptor Set）
描述符集是一个集合，包含了一组描述符（Descriptor），这些描述符指向了 GPU 资源（如纹理、缓冲区等）。描述符集在渲染管线中用于提供着色器所需的资源数据。每个描述符集可以包含多个描述符，并且在绘制调用中会将其绑定到图形管线。

### 2. 描述符池（Descriptor Pool）
描述符池是 Vulkan 中用来管理描述符集内存的结构。它的作用是定义可以分配的描述符数量和类型。描述符池在创建时需要指定其大小和能容纳的描述符类型。使用描述符池可以方便地管理和重用描述符集。

### 3. 描述符集布局（Descriptor Set Layout）
描述符集布局定义了描述符集的结构及其包含的描述符类型和绑定信息。它描述了，各个描述符在描述符集中的位置，以及这些描述符的类型（如统一缓冲区、图像采样、存储缓冲区等）和其绑定点在着色器中的使用方式。描述符集布局是固定的，一旦创建后便不能更改。

### 4. 描述符集写入（Descriptor Set Write）
描述符集写入是将实际资源（例如缓冲区或纹理）绑定到描述符集中对应的描述符。这通常通过 `vkUpdateDescriptorSets` 函数完成，通过传入具体的资源信息，将其填充到描述符集中。在渲染过程中，着色器会使用这些描述符来访问绑定好的资源。

### 这些组件之间的关系
1. **创建顺序**：
   - 首先，需要创建描述符集布局，在布局中定义描述符的类型及数量。
   - 其次，创建描述符池，并根据需要的描述符类型和数量预分配内存。
   - 然后，从描述符池中分配描述符集，分配时需要提供之前创建的描述符集布局。
   - 最后，将具体资源的信息写入描述符集。

2. **相互依赖**：
   - 描述符集布局决定了描述符集的格式和使用方式，确保在描述符集中使用的描述符类型和数量与其绑定的着色器一致。
   - 描述符池是在分配描述符集时使用的，确保你有足够的内存可供分配。
   - 描述符集写入操作依赖于描述符集和其布局，确保资源正确绑定到描述符。

### 简单示例

以下是创建描述符集、池、布局及写入的简要代码示例：

```c
// 1. 描述符集布局
VkDescriptorSetLayoutBinding layoutBinding = {};
layoutBinding.binding = 0; // 绑定点
layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 描述符类型
layoutBinding.descriptorCount = 1; // 描述符数量
layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // 使用阶段
layoutBinding.pImmutableSamplers = nullptr; // 可选，不适用这里

VkDescriptorSetLayoutCreateInfo layoutInfo = {};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &layoutBinding;

VkDescriptorSetLayout descriptorSetLayout;
vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);

// 2. 描述符池
VkDescriptorPoolSize poolSize = {};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 描述符类型
poolSize.descriptorCount = 1; // 描述符数量

VkDescriptorPoolCreateInfo poolInfo = {};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
poolInfo.maxSets = 1; // 最多分配一个描述符集

VkDescriptorPool descriptorPool;
vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

// 3. 分配描述符集
VkDescriptorSetAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = 1;
allocInfo.pSetLayouts = &descriptorSetLayout; // 使用的布局

VkDescriptorSet descriptorSet;
vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

// 4. 更新描述符集（写入）
VkDescriptorBufferInfo bufferInfo = {};
bufferInfo.buffer = uniformBuffer; // 指向实际的缓冲区
bufferInfo.offset = 0;
bufferInfo.range = sizeof(UniformBufferObject); // 缓冲区大小

VkWriteDescriptorSet descriptorWrite = {};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSet; // 目标描述符集
descriptorWrite.dstBinding = 0; // 绑定位置
descriptorWrite.dstArrayElement = 0;
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // 描述符类型
descriptorWrite.descriptorCount = 1;
descriptorWrite.pBufferInfo = &bufferInfo; // 实际绑定的缓冲区信息

vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

### 小结
- **描述符集布局**定义了描述符集的结构。
- **描述符池**提供了描述符集的内存管理。
- **描述符集**实际存储和引用 GPU 资源。
- **描述符集写入**将资源信息绑定到描述符集中。

这四者一同构成了 Vulkan 处理资源和着色器间通信的基本模型。如果有进一步的疑问或者需要更具体的例子，请随时问我！

*/