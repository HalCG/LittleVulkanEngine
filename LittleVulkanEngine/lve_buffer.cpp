/*
 * Encapsulates a vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

#include "lve_buffer.h"

 // std
#include <cassert>
#include <cstring>

namespace lve {

	/**
	 * 返回与设备兼容所需的最小实例大小minOffsetAlignment
	 *
	 * @param instanceSize 实例的大小
	 * @param minOffsetAlignment 偏移量成员所需的最小对齐方式（以字节为单位）（例如
	 * minUniformBufferOffsetAlignment)
	 *
	 * @return 缓冲区映射调用的VkResult
	 */
	VkDeviceSize LVEBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
		if (minOffsetAlignment > 0) {
			return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
		}
		return instanceSize;
	}

	LVEBuffer::LVEBuffer(
		LVEDevice& device,
		VkDeviceSize instanceSize,
		uint32_t instanceCount,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize minOffsetAlignment)
		: lveDevice{ device },
		instanceSize{ instanceSize },
		instanceCount{ instanceCount },
		usageFlags{ usageFlags },
		memoryPropertyFlags{ memoryPropertyFlags } 
	{
		alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
		bufferSize = alignmentSize * instanceCount;
		device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer, memory);
	}

	LVEBuffer::~LVEBuffer() {
		unmap();
		vkDestroyBuffer(lveDevice.device(), buffer, nullptr);
		vkFreeMemory(lveDevice.device(), memory, nullptr);
	}

	//将缓冲区的内存映射到 CPU 可访问的地址。
	VkResult LVEBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
		assert(buffer && memory && "Called map on buffer before create");
		//if (size == VK_WHOLE_SIZE) {
		//	return vkMapMemory(lveDevice.device(), memory, 0, bufferSize, 0, &mapped);
		//}
		return vkMapMemory(lveDevice.device(), memory, offset, size, 0, &mapped);
	}

	// 解除映射。
	void LVEBuffer::unmap() {
		if (mapped) {
			vkUnmapMemory(lveDevice.device(), memory);
			mapped = nullptr;
		}
	}

	// 将数据复制到已映射的缓冲区内。
	void LVEBuffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
		assert(mapped && "Cannot copy to unmapped buffer");

		if (size == VK_WHOLE_SIZE) {
			memcpy(mapped, data, bufferSize);
		}
		else {
			char* memOffset = (char*)mapped;
			memOffset += offset;
			memcpy(memOffset, data, size);
		}
	}

	// 方法用于将内存范围的内容同步到 GPU，这对于非一致性内存是必要的。
	VkResult LVEBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(lveDevice.device(), 1, &mappedRange);
	}

	// 方法使 CPU 能够看到 GPU 修改的内容，也是用于非一致性内存。
	VkResult LVEBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(lveDevice.device(), 1, &mappedRange);
	}

	// 创建描述符缓冲区信息
	VkDescriptorBufferInfo LVEBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
		return VkDescriptorBufferInfo{
			buffer,
			offset,
			size,
		};
	}

	/**
	 * 将“instanceSize”字节数据复制到索引偏移量处的映射缓冲区 *alignmentSize
	 *
	 * @param data 指向要复制的数据的指针
	 * @param index 用于偏移量计算
	 *
	 */
	void LVEBuffer::writeToIndex(void* data, int index) {
		writeToBuffer(data, instanceSize, index * alignmentSize);
	}

	/**
	 * 刷新索引处的内存范围 * 缓冲区的alignmentSize，使其对设备可见
	 *
	 * @param index 用于偏移量计算
	 *
	 */
	VkResult LVEBuffer::flushIndex(int index) { return flush(alignmentSize, index * alignmentSize); }

	/**
	 * 创建缓冲区信息描述符
	 *
	 * @param index 指定索引给定的区域 *alignmentSize
	 *
	 * @return VkDescriptorBufferInfo 例如在索引处
	 */
	VkDescriptorBufferInfo LVEBuffer::descriptorInfoForIndex(int index) {
		return descriptorInfo(alignmentSize, index * alignmentSize);
	}

	/**
	 * 使缓冲区的内存范围无效以使其对主机可见
	 *
	 * @note 仅适用于非一致性内存
	 *
	 * @paramindex 指定要失效的区域：index *alignmentSize
	 *
	 * @return 无效调用的VkResult
	 */
	VkResult LVEBuffer::invalidateIndex(int index) {
		return invalidate(alignmentSize, index * alignmentSize);
	}

}  // namespace lve
