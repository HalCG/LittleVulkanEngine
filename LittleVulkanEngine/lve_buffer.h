#pragma once

#include "lve_device.h"

namespace lve {
	/**
	 * 通用 Vulkan Buffer 封装。
	 * 职责：分配缓冲与内存；可选 map 后 CPU 写入；flush/invalidate；生成 DescriptorBufferInfo。
	 *
	 * 典型用法：
	 * - UBO：HOST_VISIBLE，每帧 map 一次后反复 writeToBuffer + flush
	 * - 顶点/索引：先 HOST 可见 staging，再 copy 到 DEVICE_LOCAL（见 LVEModel）
	 *
	 * instanceSize × instanceCount 可做成「多段」缓冲；alignmentSize 满足 minOffsetAlignment。
	 */
	class LVEBuffer {
	public:
		LVEBuffer(
			LVEDevice& device,
			VkDeviceSize instanceSize,
			uint32_t instanceCount,
			VkBufferUsageFlags usageFlags,
			VkMemoryPropertyFlags memoryPropertyFlags,
			VkDeviceSize minOffsetAlignment = 1);
		~LVEBuffer();

		LVEBuffer(const LVEBuffer&) = delete;
		LVEBuffer& operator=(const LVEBuffer&) = delete;

		VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();

		void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		/** HOST_VISIBLE 但非 HOST_COHERENT 时需 flush，否则 GPU 可能读到旧数据 */
		VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		void writeToIndex(void* data, int index);
		VkResult flushIndex(int index);
		VkDescriptorBufferInfo descriptorInfoForIndex(int index);
		VkResult invalidateIndex(int index);

		VkBuffer getBuffer() const { return buffer; }
		void* getMappedMemory() const { return mapped; }
		uint32_t getInstanceCount() const { return instanceCount; }
		VkDeviceSize getInstanceSize() const { return instanceSize; }
		VkDeviceSize getAlignmentSize() const { return instanceSize; }
		VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
		VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
		VkDeviceSize getBufferSize() const { return bufferSize; }

	private:
		static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

		LVEDevice& lveDevice;
		void* mapped = nullptr;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkDeviceSize bufferSize;
		uint32_t instanceCount;
		VkDeviceSize instanceSize;
		VkDeviceSize alignmentSize;
		VkBufferUsageFlags usageFlags;
		VkMemoryPropertyFlags memoryPropertyFlags;
	};

}  // namespace lve
