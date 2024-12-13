#include "lve_model.h"

//std
#include <cassert>

namespace lve {
	LVEModel::LVEModel(LVEDevice& device, const std::vector<Vertex>& vertices) : lveDevice{ device } {
		createVertexBuffers(vertices);
	}

	LVEModel::~LVEModel() {
		//��ģ�Ͷ�������ʱ������ Vulkan ��Դ������������ٶ��㻺�������ͷ�������ڴ档
		vkDestroyBuffer(lveDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(lveDevice.device(), vertexBufferMemory, nullptr);
	}

	void LVEModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >=3 && "Vertex count must be at least 3!");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
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
	}

	void LVEModel::draw(VkCommandBuffer commandBuffer) {//layout error draw->bind
		//�÷�����ָ������������У����� Vulkan ���� vkCmdDraw ������ģ�ͣ�ʹ�õĶ�������Ϊ vertexCount��
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}

	void LVEModel::bind(VkCommandBuffer commandBuffer) {//layout error bind->draw
		//�Ѷ��㻺�����󶨵�����������Ա�����Ļ���������Է��ʸû�������
		VkBuffer buffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
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