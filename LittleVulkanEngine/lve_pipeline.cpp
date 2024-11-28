#include "lve_pipeline.h"
#include "lve_model.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace lve {
	LVEPipeline::LVEPipeline(
		LVEDevice& device,
		const std::string& vertFilePath,
		const std::string& fragFilePath,
		const PipelineConfigInfo& configInfo) :lveDevice(device) {
		createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
	}

	LVEPipeline::~LVEPipeline() {
		vkDestroyShaderModule(lveDevice.device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(lveDevice.device(), fragShaderModule, nullptr);
		vkDestroyPipeline(lveDevice.device(), graphicsPipeline, nullptr);
	}

	std::vector<char> LVEPipeline::readFile(const std::string& filePath) {
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);//ate 文件打开时立即定位到末尾，以便定位到文件大小

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filePath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
	void LVEPipeline::createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath, const PipelineConfigInfo& configInfo) {
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE && 
			"Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
		assert(configInfo.renderPass != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline:: no renderPass provided in configInfo");
		auto vectCode = readFile(vertFilePath);
		auto fragCode = readFile(fragFilePath);

		createShaderModule(vectCode, &vertShaderModule);
		createShaderModule(fragCode, &fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		auto bindingDescriptions = LVEModel::Vertex::getBindingDescriptions();
		auto attributeDescriptions = LVEModel::Vertex::gettAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		////从configInfo 里面移出来
		//VkPipelineViewportStateCreateInfo viewportInfo{};
		//viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		//viewportInfo.viewportCount = 1;
		//viewportInfo.pViewports = &configInfo.viewport;
		//viewportInfo.scissorCount = 1;
		//viewportInfo.pScissors = &configInfo.scissor;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;

		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		//pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;//disable error
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(
			lveDevice.device(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline");
		}
	}

	void LVEPipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(lveDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module");
		}
	}

	void LVEPipeline::bind(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	void LVEPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t width, uint32_t height) {
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.viewport.x = 0.0f;
		configInfo.viewport.y = 0.0f;
		configInfo.viewport.width = static_cast<float>(width);
		configInfo.viewport.height = static_cast<float>(height);
		configInfo.viewport.minDepth = 0.0f;
		configInfo.viewport.maxDepth = 1.0f;

		configInfo.scissor.offset = { 0, 0 };
		configInfo.scissor.extent = { width, height };

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = &configInfo.viewport;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = &configInfo.scissor;

		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional
		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional
		configInfo.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional
		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};  // Optional
		configInfo.depthStencilInfo.back = {};   // Optional
	}
}

//绘制三角形时的错误信息：
/*
validation layer: Validation Error: [ UNASSIGNED-GeneralParameterError-UnrecognizedBool32 ] | MessageID = 0xa320b052 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].blendEnable (3435973836) is neither VK_TRUE nor VK_FALSE. Applications MUST not pass any other values than VK_TRUE or VK_FALSE into a Vulkan implementation where a VkBool32 is expected.
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-srcColorBlendFactor-parameter ] | MessageID = 0xeb9e690 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].srcColorBlendFactor (3435973836) does not fall within the begin..end range of the VkBlendFactor enumeration tokens and is not an extension added token. The Vulkan spec states: srcColorBlendFactor must be a valid VkBlendFactor value (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-srcColorBlendFactor-parameter)
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-dstColorBlendFactor-parameter ] | MessageID = 0x8db942fe | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].dstColorBlendFactor (3435973836) does not fall within the begin..end range of the VkBlendFactor enumeration tokens and is not an extension added token. The Vulkan spec states: dstColorBlendFactor must be a valid VkBlendFactor value (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-dstColorBlendFactor-parameter)
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-colorBlendOp-parameter ] | MessageID = 0x7978ef4f | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].colorBlendOp (3435973836) does not fall within the begin..end range of the VkBlendOp enumeration tokens and is not an extension added token. The Vulkan spec states: colorBlendOp must be a valid VkBlendOp value (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-colorBlendOp-parameter)
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-srcAlphaBlendFactor-parameter ] | MessageID = 0x4a7f8ed8 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].srcAlphaBlendFactor (3358545944) does not fall within the begin..end range of the VkBlendFactor enumeration tokens and is not an extension added token. The Vulkan spec states: srcAlphaBlendFactor must be a valid VkBlendFactor value (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-srcAlphaBlendFactor-parameter)
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-dstAlphaBlendFactor-parameter ] | MessageID = 0x53ab1c8f | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].dstAlphaBlendFactor (496) does not fall within the begin..end range of the VkBlendFactor enumeration tokens and is not an extension added token. The Vulkan spec states: dstAlphaBlendFactor must be a valid VkBlendFactor value (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-dstAlphaBlendFactor-parameter)
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-alphaBlendOp-parameter ] | MessageID = 0x48f9f9b | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].alphaBlendOp (4127412873) does not fall within the begin..end range of the VkBlendOp enumeration tokens and is not an extension added token. The Vulkan spec states: alphaBlendOp must be a valid VkBlendOp value (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-alphaBlendOp-parameter)
validation layer: Validation Error: [ VUID-VkPipelineColorBlendAttachmentState-colorWriteMask-parameter ] | MessageID = 0xb8f9c032 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].colorWriteMask contains flag bits (0x36b2) which are not recognized members of VkColorComponentFlagBits. The Vulkan spec states: colorWriteMask must be a valid combination of VkColorComponentFlagBits values (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-VkPipelineColorBlendAttachmentState-colorWriteMask-parameter)
failed to submit draw command buffer!
*/
/*
好的，我们来分析你提供的 Vulkan 错误日志。这个日志包含了多个验证错误，主要是在调用 `vkCreateGraphicsPipelines()` 时，与颜色混合状态的参数设置有关。下面是每个错误的详细分析：

### 1. 无效的_bool32_值
```
验证错误: [ UNASSIGNED-GeneralParameterError-UnrecognizedBool32 ] | MessageID = 0xa320b052 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].blendEnable (3435973836) 不是 VK_TRUE 或 VK_FALSE。
```
- **问题**: `blendEnable` 字段应该是布尔值（`VK_TRUE` 或 `VK_FALSE`），但接收到一个无效的值（3435973836）。
- **解决方案**: 确保将 `blendEnable` 设置为 `VK_TRUE` 或 `VK_FALSE`。

### 2. 无效的源颜色混合因子
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-srcColorBlendFactor-parameter ] | MessageID = 0xeb9e690 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].srcColorBlendFactor (3435973836) 不在 VkBlendFactor 枚举的有效范围内，且不是扩展添加的标记。
```
- **问题**: `srcColorBlendFactor` 应该是有效的 `VkBlendFactor` 枚举值，但设置了一个无效的数字（3435973836）。
- **解决方案**: 检查赋给 `srcColorBlendFactor` 的值，确保它符合定义的 `VkBlendFactor` 值（例如，`VK_BLEND_FACTOR_ZERO`、`VK_BLEND_FACTOR_ONE` 等）。

### 3. 无效的目标颜色混合因子
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-dstColorBlendFactor-parameter ] | MessageID = 0x8db942fe | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].dstColorBlendFactor (3435973836) 不在 VkBlendFactor 枚举的有效范围内，且不是扩展添加的标记。
```
- **问题**: `dstColorBlendFactor` 同样是设置为无效值。
- **解决方案**: 确保 `dstColorBlendFactor` 被赋予一个有效的 `VkBlendFactor` 值。

### 4. 无效的颜色混合操作
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-colorBlendOp-parameter ] | MessageID = 0x7978ef4f | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].colorBlendOp (3435973836) 不在 VkBlendOp 枚举的有效范围内，且不是扩展添加的标记。
```
- **问题**: `colorBlendOp` 参数设置为无效值。
- **解决方案**: 为 `colorBlendOp` 赋予有效的 `VkBlendOp` 值。

### 5. 无效的源 alpha 混合因子
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-srcAlphaBlendFactor-parameter ] | MessageID = 0x4a7f8ed8 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].srcAlphaBlendFactor (3358545944) 不在 VkBlendFactor 枚举的有效范围内，且不是扩展添加的标记。
```
- **问题**: `srcAlphaBlendFactor` 设置为无效值。
- **解决方案**: 确保 `srcAlphaBlendFactor` 被赋予一个有效的 `VkBlendFactor` 值。

### 6. 无效的目标 alpha 混合因子
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-dstAlphaBlendFactor-parameter ] | MessageID = 0x53ab1c8f | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].dstAlphaBlendFactor (496) 不在 VkBlendFactor 枚举的有效范围内，且不是扩展添加的标记。
```
- **问题**: `dstAlphaBlendFactor` 设置为无效值。
- **解决方案**: 将 `dstAlphaBlendFactor` 设置为有效的 `VkBlendFactor` 值。

### 7. 无效的 alpha 混合操作
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-alphaBlendOp-parameter ] | MessageID = 0x48f9f9b | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].alphaBlendOp (4127412873) 不在 VkBlendOp 枚举的有效范围内，且不是扩展添加的标记。
```
- **问题**: `alphaBlendOp` 为无效值。
- **解决方案**: 为 `alphaBlendOp` 设置有效的 `VkBlendOp` 值。

### 8. 无效的颜色写掩码
```
验证错误: [ VUID-VkPipelineColorBlendAttachmentState-colorWriteMask-parameter ] | MessageID = 0xb8f9c032 | vkCreateGraphicsPipelines(): pCreateInfos[0].pColorBlendState->pAttachments[0].colorWriteMask 包含 (0x36b2) 无法识别的 VkColorComponentFlagBits 成员。
```
- **问题**: `colorWriteMask` 中包含了无效的标志位。
- **解决方案**: 将 `colorWriteMask` 设置为有效的 `VkColorComponentFlagBits` 组合（例如，`VK_COLOR_COMPONENT_R_BIT`、`VK_COLOR_COMPONENT_G_BIT` 等）。

### 总结
要解决这些错误，请仔细检查赋值给颜色混合状态各个参数的值，确保它们符合 Vulkan 规范中所要求的有效值。如果不确定正确的值，可以查阅你所使用的 Vulkan 版本的文档。这将有助于你纠正代码并消除验证错误。如果还有其他问题，随时问我！

*/