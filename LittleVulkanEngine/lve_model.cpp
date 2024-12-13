#include "lve_model.h"

//std
#include <cassert>

namespace lve {
	LVEModel::LVEModel(LVEDevice& device, const std::vector<Vertex>& vertices) : lveDevice{ device } {
		createVertexBuffers(vertices);
	}

	LVEModel::~LVEModel() {
		//当模型对象被销毁时，清理 Vulkan 资源，具体包括销毁顶点缓冲区并释放其相关内存。
		vkDestroyBuffer(lveDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(lveDevice.device(), vertexBufferMemory, nullptr);
	}

	void LVEModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >=3 && "Vertex count must be at least 3!");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, //顶点缓冲区
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//缓冲区是可被主机访问和一致性的（host-visible and coherent）
			vertexBuffer,
			vertexBufferMemory);

		void* data;
		vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);//映射内存，以便可以将顶点数据复制到缓冲区中。
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(lveDevice.device(), vertexBufferMemory);
	}

	void LVEModel::draw(VkCommandBuffer commandBuffer) {//layout error draw->bind
		//该方法在指定的命令缓冲区中，调用 Vulkan 函数 vkCmdDraw 来绘制模型，使用的顶点数量为 vertexCount。
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}

	void LVEModel::bind(VkCommandBuffer commandBuffer) {//layout error bind->draw
		//把顶点缓冲区绑定到命令缓冲区，以便后续的绘制命令可以访问该缓冲区。
		VkBuffer buffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	}
	
	//用于提供缓冲区的绑定信息
	//获取顶点绑定描述: 返回与顶点输入绑定相关的描述，包括绑定位置、步幅和输入速率设置。这里的步幅定义了每个顶点数据结构的大小。
	std::vector<VkVertexInputBindingDescription> LVEModel::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//TODO
		return bindingDescriptions;
	}

	//提供属性（如位置和颜色）的具体格式信息
	//获取顶点属性描述: 返回顶点输入属性的描述。其中包括顶点的位置信息和颜色信息的格式，绑定位置，偏移量等
	std::vector<VkVertexInputAttributeDescription> LVEModel::Vertex::gettAttributeDescriptions() {
		std::vector <VkVertexInputAttributeDescription> attributeDescriptions(2);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		//此处颜色与顶点位置可以用两个不同bind，也可以使用一个绑定，不同属性偏移
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
		/*
		return	{
			{0,0,VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)},
			{0,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, color)}};
		*/
	}
}


/*
在 Vulkan 中，`vertexBuffer` 和 `vertexBufferMemory` 是两个密切相关的概念，分别代表顶点缓冲区和其所映射的内存。它们的关系和作用如下：

### 1. `vertexBuffer`
- **定义**: `vertexBuffer` 是一个 Vulkan 缓冲区对象（`VkBuffer`），用于存储顶点数据的块。
- **功能**:
  - 作为 GPU 可以访问的数据存储区域，当需要渲染三维模型时，GPU 从这里读取顶点信息。
  - 在绘制命令中（例如 `vkCmdBindVertexBuffers`），将此缓冲区绑定到图形管线，使得后续的绘制操作能够使用这些顶点数据。

### 2. `vertexBufferMemory`
- **定义**: `vertexBufferMemory` 是一个 Vulkan 内存对象（`VkDeviceMemory`），与 `vertexBuffer` 关联，提供实际的数据存储空间。
- **功能**:
  - 存储顶点缓冲区对象（`vertexBuffer`）的数据。Vulkan 的设计使得 GPU 和 CPU 的内存管理分开，因此需要显式地分配内存以供 GPU 使用。
  - 在创建缓冲区后，必须分配内存并将其绑定到缓冲区对象，这样才能在缓冲区中存储数据（如顶点位置、颜色等）。

### 关系
- **绑定关系**: 在 Vulkan 中，缓冲区对象（如 `vertexBuffer`）需要一个内存对象（如 `vertexBufferMemory`）来实际存储数据。创建缓冲区后，通常需要通过 `vkBindBufferMemory` 函数将 `vertexBufferMemory` 绑定到 `vertexBuffer`，建立它们之间的关系。
- **数据流向**: 数据的流动通常是这样的：
  1. 分配内存 (`vertexBufferMemory`)。
  2. 创建缓冲区对象 (`vertexBuffer`)。
  3. 将内存绑定到缓冲区。
  4. 将数据（如顶点数据）写入到绑定的内存中。

### 示例
在 `LVEModel::createVertexBuffers` 函数中，先创建缓冲区，然后映射内存以便于数据的写入：

```cpp
// 创建顶点缓冲
lveDevice.createBuffer(
	bufferSize, // 缓冲区的大小
	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // 使用类型：顶点缓冲区
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // 内存属性
	vertexBuffer, // 输出的缓冲区
	vertexBufferMemory // 输出的内存
);

// 映射内存，准备写入数据
void* data;
vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
memcpy(data, vertices.data(), static_cast<size_t>(bufferSize)); // 把顶点数据复制到内存中
vkUnmapMemory(lveDevice.device(), vertexBufferMemory); // 取消映射内存
```

### 总结
- `vertexBuffer` 是指向存储顶点数据缓冲区的句柄，直接用于图形管线的渲染。
- `vertexBufferMemory` 是与该缓冲区对象关联的内存，实际存储顶点数据。两者结合使用，实现了 Vulkan 中数据的高效管理和访问。

*/

/*
`getBindingDescriptions` 和 `getAttributeDescriptions` 方法都用于描述顶点输入的格式，但它们各自关注的方面不同。以下是这两个方法的区别，以及为什么需要在映射内存后调用 `vkUnmapMemory` 的解释。

### 1. `getBindingDescriptions` 和 `getAttributeDescriptions` 的区别

#### `getBindingDescriptions`
- **功能**: 描述 Vulkan 中的顶点输入绑定（`VkVertexInputBindingDescription`）。
- **返回内容**:
  - **binding**: 绑定的位置，一个好友的标识符（通常为 0）。
  - **stride**: 每个顶点数据结构的字节大小（例如，结构体 `Vertex` 的大小）。该值用于表示从一个顶点到下一个顶点之间的字节数。
  - **inputRate**: 输入速率，它指示数据是每个顶点传递一次（`VK_VERTEX_INPUT_RATE_VERTEX`），还是一次传递整个顶点缓冲区（`VK_VERTEX_INPUT_RATE_INSTANCE`），通常用于实例化渲染。

##### 示例
```cpp
std::vector<VkVertexInputBindingDescription> LVEModel::Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0; // 绑定位置
	bindingDescriptions[0].stride = sizeof(Vertex); // 步幅
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 输入速率
	return bindingDescriptions;
}
```

#### `getAttributeDescriptions`
- **功能**: 描述每个顶点的属性（`VkVertexInputAttributeDescription`），用于指定缓冲区中每个属性的格式。
- **返回内容**:
  - **binding**: 关联的顶点输入绑定的位置（通常与绑定描述相对应）。
  - **location**: 在着色器中对应的输入位置，这个位置用于在着色器内引用顶点属性。
  - **format**: 数据格式，如`VK_FORMAT_R32G32_SFLOAT`（代表一个包含 X 和 Y 坐标的二维点），或 `VK_FORMAT_R32G32B32_SFLOAT`（代表包含 RGB 颜色的三维颜色）。
  - **offset**: 属性在顶点结构体中的偏移量，指示当前属性在缓冲区中相对于顶点数据开头的偏移量。

##### 示例
```cpp
std::vector<VkVertexInputAttributeDescription> LVEModel::Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
	attributeDescriptions[0].binding = 0; // 绑定位置
	attributeDescriptions[0].location = 0; // 着色器输入位置
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // 数据格式
	attributeDescriptions[0].offset = offsetof(Vertex, position); // 偏移

	attributeDescriptions[1].binding = 0; // 绑定位置
	attributeDescriptions[1].location = 1; // 着色器输入位置
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // 数据格式
	attributeDescriptions[1].offset = offsetof(Vertex, color); // 偏移

	return attributeDescriptions;
}
```

### 2. 映射内存和调用 `vkUnmapMemory` 的原因

在 Vulkan 中，从主机（CPU）向设备（GPU）传输数据时，通常要映射内存以便于访问。以下是映射内存后调用 `vkUnmapMemory` 的原因：

- **映射内存 (`vkMapMemory`)**: 这一步骤允许 CPU 访问 GPU 内存并操作数据。通过映射，我们可以获得实际存储数据的地址指针（如 `void* data`），然后就可以直接使用这个指针来读取或写入数据。例如，在顶点缓冲区中，我们将顶点数据从 CPU 复制到 GPU 的内存空间。

- **取消映射内存 (`vkUnmapMemory`)**:
  - 在完成数据写入后需要调用 `vkUnmapMemory`，这是为了通知 Vulkan 内存访问结束，确保数据一致性并释放对该内存的访问。
  - Vulkan 是一种显式的图形 API，使用者需要明确管理内存的映射状态。未能取消映射可能导致逻辑错误、内存泄漏或资源冲突等问题，因为 GPU 可能无法正确地访问更新后的数据。

### 小结
- `getBindingDescriptions` 用于提供缓冲区的绑定信息，而 `getAttributeDescriptions` 提供属性（如位置和颜色）的具体格式信息。
- 映射内存后调用 `vkUnmapMemory` 是必要的，以确保数据正确写入，并告知 Vulkan 内存的访问结束，从而保护内存的有效性和一致性。

*/