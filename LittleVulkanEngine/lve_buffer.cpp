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
	 * �������豸�����������Сʵ����СminOffsetAlignment
	 *
	 * @param instanceSize ʵ���Ĵ�С
	 * @param minOffsetAlignment ƫ������Ա�������С���뷽ʽ�����ֽ�Ϊ��λ��������
	 * minUniformBufferOffsetAlignment)
	 *
	 * @return ������ӳ����õ�VkResult
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

	//�����������ڴ�ӳ�䵽 CPU �ɷ��ʵĵ�ַ��
	VkResult LVEBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
		assert(buffer && memory && "Called map on buffer before create");
		//if (size == VK_WHOLE_SIZE) {
		//	return vkMapMemory(lveDevice.device(), memory, 0, bufferSize, 0, &mapped);
		//}
		return vkMapMemory(lveDevice.device(), memory, offset, size, 0, &mapped);
	}

	// ���ӳ�䡣
	void LVEBuffer::unmap() {
		if (mapped) {
			vkUnmapMemory(lveDevice.device(), memory);
			mapped = nullptr;
		}
	}

	// �����ݸ��Ƶ���ӳ��Ļ������ڡ�
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

	// �������ڽ��ڴ淶Χ������ͬ���� GPU������ڷ�һ�����ڴ��Ǳ�Ҫ�ġ�
	VkResult LVEBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(lveDevice.device(), 1, &mappedRange);
	}

	// ����ʹ CPU �ܹ����� GPU �޸ĵ����ݣ�Ҳ�����ڷ�һ�����ڴ档
	VkResult LVEBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(lveDevice.device(), 1, &mappedRange);
	}

	// ������������������Ϣ
	VkDescriptorBufferInfo LVEBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
		return VkDescriptorBufferInfo{
			buffer,
			offset,
			size,
		};
	}

	/**
	 * ����instanceSize���ֽ����ݸ��Ƶ�����ƫ��������ӳ�仺���� *alignmentSize
	 *
	 * @param data ָ��Ҫ���Ƶ����ݵ�ָ��
	 * @param index ����ƫ��������
	 *
	 */
	void LVEBuffer::writeToIndex(void* data, int index) {
		writeToBuffer(data, instanceSize, index * alignmentSize);
	}

	/**
	 * ˢ�����������ڴ淶Χ * ��������alignmentSize��ʹ����豸�ɼ�
	 *
	 * @param index ����ƫ��������
	 *
	 */
	VkResult LVEBuffer::flushIndex(int index) { return flush(alignmentSize, index * alignmentSize); }

	/**
	 * ������������Ϣ������
	 *
	 * @param index ָ���������������� *alignmentSize
	 *
	 * @return VkDescriptorBufferInfo ������������
	 */
	VkDescriptorBufferInfo LVEBuffer::descriptorInfoForIndex(int index) {
		return descriptorInfo(alignmentSize, index * alignmentSize);
	}

	/**
	 * ʹ���������ڴ淶Χ��Ч��ʹ��������ɼ�
	 *
	 * @note �������ڷ�һ�����ڴ�
	 *
	 * @paramindex ָ��ҪʧЧ������index *alignmentSize
	 *
	 * @return ��Ч���õ�VkResult
	 */
	VkResult LVEBuffer::invalidateIndex(int index) {
		return invalidate(alignmentSize, index * alignmentSize);
	}

}  // namespace lve
