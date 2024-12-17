#include "lve_model.h"

//std
#include <cassert>

namespace lve {
	LVEModel::LVEModel(LVEDevice& device, const LVEModel::Builder& builder) : lveDevice{ device } {
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	LVEModel::~LVEModel() {
		//��ģ�Ͷ�������ʱ������ Vulkan ��Դ������������ٶ��㻺�������ͷ�������ڴ档
		vkDestroyBuffer(lveDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(lveDevice.device(), vertexBufferMemory, nullptr);

		if (hasIndexBuffer) {
			vkDestroyBuffer(lveDevice.device(), indexBuffer, nullptr);
			vkFreeMemory(lveDevice.device(), indexBufferMemory, nullptr);
		}
	}

	void LVEModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >=3 && "Vertex count must be at least 3!");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

#if 0
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, //���㻺����
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//�������ǿɱ��������ʺ�һ���Եģ�host-visible and coherent��
			vertexBuffer,
			vertexBufferMemory);

		void* data;
		vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);//ӳ���ڴ棬�Ա���Խ��������ݸ��Ƶ��������С�
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(lveDevice.device(), vertexBufferMemory);
#endif
		/*
		### ΪʲôҪ�������ο�����

			1. **���ܿ���**��  
			   - **staging buffer** ������ CPU �� GPU ֮����и�Ч�����ݴ��䡣ʹ�� `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` �� `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` ʹ�� CPU ����ֱ�ӷ��ʺ��޸���������������ݡ�ͨ�����ַ�ʽ���������ݿ��Է���ش� CPU ׼���ã�Ȼ��ֱ��д�������������
			   - �����Ķ��㻺������`vertexBuffer`��ʹ�� `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` �Ա��� GPU �����ڴ��У�ͨ�����Ի�ø��ߵ���Ⱦ���ܡ���ˣ�staging buffer ����ֱ��������Ⱦ��������Ϊһ�����ݴ���;����

			2. **�ڴ�����**��
			   - GPU �����ڴ�ͨ������������ֱ�Ӵ� CPU ���ʣ������Ҫ��ͨ�� staging buffer �Ƚ��������һ�ο�����ֱ�ӽ��������ݸ��Ƶ� GPU �����ڴ���ܻ������޷����ʻ�Ч�ʵ��µ����⡣

			3. **���ݴ����Ż�**��
			   - ����������̣������Ը�����������ݴ��䣺ȷ���������ڴ��е������Ǻ��ʵģ���������Բ�ͬ��;��CPU ������ GPU ��Ⱦ�����к��ʵ��ڴ���䡣
		*/


		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		//1. ������ʱ��������Staging Buffer��
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,//���������ڴ������
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//�������ǿɱ��������ʺ�һ���Եģ�host-visible and coherent��
			stagingBuffer,
			stagingBufferMemory);

		//2. �������ݸ��Ƶ� stagingBuffer
		void* data;
		vkMapMemory(lveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(lveDevice.device(), stagingBufferMemory);

		//3. ����һ�� GPU �Ż��Ķ��㻺���� vertexBuffer����ʹ�� VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT����ȷ������ GPU �����ڴ��У��������������Ⱦ����
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);

		//4. ����ʱ���������Ƶ����㻺����
		lveDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		vkDestroyBuffer(lveDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(lveDevice.device(), stagingBufferMemory, nullptr);
	}

	void LVEModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;
		if (!hasIndexBuffer) {
			return;
		}
		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);
		void* data;
		vkMapMemory(lveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(lveDevice.device(), stagingBufferMemory);
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBuffer,
			indexBufferMemory);
		lveDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);
		vkDestroyBuffer(lveDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(lveDevice.device(), stagingBufferMemory, nullptr);
	}

	void LVEModel::draw(VkCommandBuffer commandBuffer) {//layout error draw->bind
		//�÷�����ָ������������У�CmdDraw ������ģ��
		if (hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	void LVEModel::bind(VkCommandBuffer commandBuffer) {//layout error bind->draw
		//�Ѷ��㻺�����󶨵�����������Ա�����Ļ���������Է��ʸû�������
		VkBuffer buffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}
	
	//�����ṩ�������İ���Ϣ
	//��ȡ���������: �����붥���������ص�������������λ�á������������������á�����Ĳ���������ÿ���������ݽṹ�Ĵ�С��
	std::vector<VkVertexInputBindingDescription> LVEModel::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//TODO
		return bindingDescriptions;
	}

	//�ṩ���ԣ���λ�ú���ɫ���ľ����ʽ��Ϣ
	//��ȡ������������: ���ض����������Ե����������а��������λ����Ϣ����ɫ��Ϣ�ĸ�ʽ����λ�ã�ƫ������
	std::vector<VkVertexInputAttributeDescription> LVEModel::Vertex::gettAttributeDescriptions() {
		std::vector <VkVertexInputAttributeDescription> attributeDescriptions(2);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		//�˴���ɫ�붥��λ�ÿ�����������ͬbind��Ҳ����ʹ��һ���󶨣���ͬ����ƫ��
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
�� Vulkan �У�`vertexBuffer` �� `vertexBufferMemory` ������������صĸ���ֱ�����㻺����������ӳ����ڴ档���ǵĹ�ϵ���������£�

### 1. `vertexBuffer`
- **����**: `vertexBuffer` ��һ�� Vulkan ����������`VkBuffer`�������ڴ洢�������ݵĿ顣
- **����**:
  - ��Ϊ GPU ���Է��ʵ����ݴ洢���򣬵���Ҫ��Ⱦ��άģ��ʱ��GPU �������ȡ������Ϣ��
  - �ڻ��������У����� `vkCmdBindVertexBuffers`�������˻������󶨵�ͼ�ι��ߣ�ʹ�ú����Ļ��Ʋ����ܹ�ʹ����Щ�������ݡ�

### 2. `vertexBufferMemory`
- **����**: `vertexBufferMemory` ��һ�� Vulkan �ڴ����`VkDeviceMemory`������ `vertexBuffer` �������ṩʵ�ʵ����ݴ洢�ռ䡣
- **����**:
  - �洢���㻺��������`vertexBuffer`�������ݡ�Vulkan �����ʹ�� GPU �� CPU ���ڴ����ֿ��������Ҫ��ʽ�ط����ڴ��Թ� GPU ʹ�á�
  - �ڴ����������󣬱�������ڴ沢����󶨵��������������������ڻ������д洢���ݣ��綥��λ�á���ɫ�ȣ���

### ��ϵ
- **�󶨹�ϵ**: �� Vulkan �У������������� `vertexBuffer`����Ҫһ���ڴ������ `vertexBufferMemory`����ʵ�ʴ洢���ݡ�������������ͨ����Ҫͨ�� `vkBindBufferMemory` ������ `vertexBufferMemory` �󶨵� `vertexBuffer`����������֮��Ĺ�ϵ��
- **��������**: ���ݵ�����ͨ���������ģ�
  1. �����ڴ� (`vertexBufferMemory`)��
  2. �������������� (`vertexBuffer`)��
  3. ���ڴ�󶨵���������
  4. �����ݣ��綥�����ݣ�д�뵽�󶨵��ڴ��С�

### ʾ��
�� `LVEModel::createVertexBuffers` �����У��ȴ�����������Ȼ��ӳ���ڴ��Ա������ݵ�д�룺

```cpp
// �������㻺��
lveDevice.createBuffer(
	bufferSize, // �������Ĵ�С
	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // ʹ�����ͣ����㻺����
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // �ڴ�����
	vertexBuffer, // ����Ļ�����
	vertexBufferMemory // ������ڴ�
);

// ӳ���ڴ棬׼��д������
void* data;
vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
memcpy(data, vertices.data(), static_cast<size_t>(bufferSize)); // �Ѷ������ݸ��Ƶ��ڴ���
vkUnmapMemory(lveDevice.device(), vertexBufferMemory); // ȡ��ӳ���ڴ�
```

### �ܽ�
- `vertexBuffer` ��ָ��洢�������ݻ������ľ����ֱ������ͼ�ι��ߵ���Ⱦ��
- `vertexBufferMemory` ����û���������������ڴ棬ʵ�ʴ洢�������ݡ����߽��ʹ�ã�ʵ���� Vulkan �����ݵĸ�Ч����ͷ��ʡ�

*/

/*
`getBindingDescriptions` �� `getAttributeDescriptions` ����������������������ĸ�ʽ�������Ǹ��Թ�ע�ķ��治ͬ�������������������������Լ�Ϊʲô��Ҫ��ӳ���ڴ����� `vkUnmapMemory` �Ľ��͡�

### 1. `getBindingDescriptions` �� `getAttributeDescriptions` ������

#### `getBindingDescriptions`
- **����**: ���� Vulkan �еĶ�������󶨣�`VkVertexInputBindingDescription`����
- **��������**:
  - **binding**: �󶨵�λ�ã�һ�����ѵı�ʶ����ͨ��Ϊ 0����
  - **stride**: ÿ���������ݽṹ���ֽڴ�С�����磬�ṹ�� `Vertex` �Ĵ�С������ֵ���ڱ�ʾ��һ�����㵽��һ������֮����ֽ�����
  - **inputRate**: �������ʣ���ָʾ������ÿ�����㴫��һ�Σ�`VK_VERTEX_INPUT_RATE_VERTEX`��������һ�δ����������㻺������`VK_VERTEX_INPUT_RATE_INSTANCE`����ͨ������ʵ������Ⱦ��

##### ʾ��
```cpp
std::vector<VkVertexInputBindingDescription> LVEModel::Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0; // ��λ��
	bindingDescriptions[0].stride = sizeof(Vertex); // ����
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // ��������
	return bindingDescriptions;
}
```

#### `getAttributeDescriptions`
- **����**: ����ÿ����������ԣ�`VkVertexInputAttributeDescription`��������ָ����������ÿ�����Եĸ�ʽ��
- **��������**:
  - **binding**: �����Ķ�������󶨵�λ�ã�ͨ������������Ӧ����
  - **location**: ����ɫ���ж�Ӧ������λ�ã����λ����������ɫ�������ö������ԡ�
  - **format**: ���ݸ�ʽ����`VK_FORMAT_R32G32_SFLOAT`������һ������ X �� Y ����Ķ�ά�㣩���� `VK_FORMAT_R32G32B32_SFLOAT`��������� RGB ��ɫ����ά��ɫ����
  - **offset**: �����ڶ���ṹ���е�ƫ������ָʾ��ǰ�����ڻ�����������ڶ������ݿ�ͷ��ƫ������

##### ʾ��
```cpp
std::vector<VkVertexInputAttributeDescription> LVEModel::Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
	attributeDescriptions[0].binding = 0; // ��λ��
	attributeDescriptions[0].location = 0; // ��ɫ������λ��
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // ���ݸ�ʽ
	attributeDescriptions[0].offset = offsetof(Vertex, position); // ƫ��

	attributeDescriptions[1].binding = 0; // ��λ��
	attributeDescriptions[1].location = 1; // ��ɫ������λ��
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // ���ݸ�ʽ
	attributeDescriptions[1].offset = offsetof(Vertex, color); // ƫ��

	return attributeDescriptions;
}
```

### 2. ӳ���ڴ�͵��� `vkUnmapMemory` ��ԭ��

�� Vulkan �У���������CPU�����豸��GPU����������ʱ��ͨ��Ҫӳ���ڴ��Ա��ڷ��ʡ�������ӳ���ڴ����� `vkUnmapMemory` ��ԭ��

- **ӳ���ڴ� (`vkMapMemory`)**: ��һ�������� CPU ���� GPU �ڴ沢�������ݡ�ͨ��ӳ�䣬���ǿ��Ի��ʵ�ʴ洢���ݵĵ�ַָ�루�� `void* data`����Ȼ��Ϳ���ֱ��ʹ�����ָ������ȡ��д�����ݡ����磬�ڶ��㻺�����У����ǽ��������ݴ� CPU ���Ƶ� GPU ���ڴ�ռ䡣

- **ȡ��ӳ���ڴ� (`vkUnmapMemory`)**:
  - ���������д�����Ҫ���� `vkUnmapMemory`������Ϊ��֪ͨ Vulkan �ڴ���ʽ�����ȷ������һ���Բ��ͷŶԸ��ڴ�ķ��ʡ�
  - Vulkan ��һ����ʽ��ͼ�� API��ʹ������Ҫ��ȷ�����ڴ��ӳ��״̬��δ��ȡ��ӳ����ܵ����߼������ڴ�й©����Դ��ͻ�����⣬��Ϊ GPU �����޷���ȷ�ط��ʸ��º�����ݡ�

### С��
- `getBindingDescriptions` �����ṩ�������İ���Ϣ���� `getAttributeDescriptions` �ṩ���ԣ���λ�ú���ɫ���ľ����ʽ��Ϣ��
- ӳ���ڴ����� `vkUnmapMemory` �Ǳ�Ҫ�ģ���ȷ��������ȷд�룬����֪ Vulkan �ڴ�ķ��ʽ������Ӷ������ڴ����Ч�Ժ�һ���ԡ�

*/


/*
�����ṩ�Ĵ����У������ַ������ڴ�������� Vulkan �Ķ��㻺���������������ߵ���ϸ�Ƚϣ�

### ��ע�͵��ķ�����

```cpp
#if 0
	lveDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, //���㻺����
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//�������ǿɱ��������ʺ�һ���Եģ�host-visible and coherent��
		vertexBuffer,
		vertexBufferMemory);

	void* data;
	vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);//ӳ���ڴ棬�Ա���Խ��������ݸ��Ƶ��������С�
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(lveDevice.device(), vertexBufferMemory);
#endif
```

#### ˵����
1. **�������㻺����**�����ַ���ֱ��ʹ�� `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` ����һ�����㻺��������ζ��������������������������ݵĴ洢��
2. **�ڴ�����**��ʹ�� `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`��������˻��������Ա��������ʣ�����һ�µģ��������Է������ CPU ��ֱ��д�����ݡ�
3. **ֱ�Ӹ�������**��ͨ��ӳ���ڴ� (`vkMapMemory`) ������ֱ�ӽ����ݴ� `vertices` ���鸴�Ƶ� `vertexBuffer` �С�

#### ȱ�㣺
- ����ʹ�� `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`�������Ļ������������ܲ��ߡ����ֱ���ڴ˻������ڴ���ִ�� GPU �������ɱ��ϸߣ���Ϊ���ݱ��뾭�� CPU �� GPU ֮��Ĵ��䡣

### û��ע�͵ķ�����

```cpp
VkBuffer stagingBuffer;
VkDeviceMemory stagingBufferMemory;

lveDevice.createBuffer(
	bufferSize,
	VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //���㻺����
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,//�������ǿɱ��������ʺ�һ���Եģ�host-visible and coherent��
	stagingBuffer,
	stagingBufferMemory);

void* data;
vkMapMemory(lveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
vkUnmapMemory(lveDevice.device(), stagingBufferMemory);
lveDevice.createBuffer(
	bufferSize,
	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	vertexBuffer,
	vertexBufferMemory);

lveDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
vkDestroyBuffer(lveDevice.device(), stagingBuffer, nullptr);
vkFreeMemory(lveDevice.device(), stagingBufferMemory, nullptr);
```

#### ˵����
1. **������ʱ��������Staging Buffer��**�����ȴ���һ���������ݴ������ʱ������ `stagingBuffer`����ʹ�� `VK_BUFFER_USAGE_TRANSFER_SRC_BIT` ��Ϊ����;�����������ڴ��������
2. **�������ݵ���ʱ������**�����Ƚ��������ݸ��Ƶ� `stagingBuffer`��Ȼ�󴴽�һ�� GPU �Ż��Ķ��㻺���� `vertexBuffer`����ʹ�� `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`����ȷ������ GPU �����ڴ��У��������������Ⱦ���ܡ�
3. **����ʱ���������Ƶ����㻺����**��ʹ�� `lveDevice.copyBuffer` �����ݴ� `stagingBuffer` ���Ƶ� `vertexBuffer`����һ����ͨ���� GPU ֮����У�Ч�ʸ��ߡ�
4. **������ʱ������**��������� `stagingBuffer` ���ͷ����ڴ棬ȷ��û���ڴ�й©��

#### �ŵ㣺
- **�����Ż�**��ͨ��ʹ�� staging buffer������ȷ������������ GPU �ڴ��У�`vertexBuffer`�����Ӷ���ߺ�����Ⱦ��Ч�ʡ�
- **���� CPU �� GPU �����ķ���**��ʹ�� staging buffer ������Ч���� CPU ����׼���� GPU ��Ⱦ����֮����룬�Ӷ������������ܡ�

### �ܽ᣺

��ע�͵��ķ����ʺϿ��ٲ��Ժͼ򻯵ĳ��������������Ż��ϲ���û��ע�͵ķ����á�����������ʱ������������ȷ�������ݴ�����̣����� CPU �� GPU �Ĳ���Ч����󻯣����ʺ���ʵ�ʿ����е�ʹ�������

*/