#pragma once

#include <string>
#include <vector>
#include "lve_device.h"


namespace lve{
	/**
	 * 图形管线固定功能状态集合（不含着色器模块本身）。
	 * 由 defaultPipelineConfigInfo 填默认值；各 RenderSystem 再改 renderPass / layout / 混合等。
	 * viewport/scissor 使用动态状态，每帧在 beginSwapChainRenderPass 里设置。
	 */
	struct PipelineConfigInfo {
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		VkPipelineViewportStateCreateInfo viewportInfo{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};

		std::vector<VkDynamicState> dynamicStateEnables{};
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

		VkPipelineLayout pipelineLayout = nullptr;  // 描述符集 + push constant 布局
		VkRenderPass renderPass = nullptr;         // 必须与实际使用的 RenderPass 兼容
		uint32_t subpass = 0;
	};

	/**
	 * 加载 vert/frag SPIR-V，创建 VkPipeline。
	 * bind() 仅把管线绑到 command buffer；实际 draw 由 RenderSystem 完成。
	 */
	class LVEPipeline {
	public:
		LVEPipeline() = default;
		LVEPipeline(
			LVEDevice& device,
			const std::string& vertFilePath,
			const std::string& fragFilePath,
			const PipelineConfigInfo& configInfo);

		~LVEPipeline();
		
		LVEPipeline(const LVEPipeline&) = delete;
		void operator=(const LVEPipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);

		/** 三角形列表、背面剔除、深度测试、动态 viewport/scissor 等常用默认 */
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		/** 点光源 Billboard 等半透明绘制时开启 srcAlpha 混合 */
		static void enableAlphaBlending(PipelineConfigInfo& configInfo);

	private:
		static std::vector<char> readFile(const std::string& filePath);

		void createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath, const PipelineConfigInfo& configInfo);

		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		LVEDevice& lveDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}
