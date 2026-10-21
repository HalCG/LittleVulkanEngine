#include "lve_pipeline.h"
#include "lve_model.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace lve {
	/*
		LVEPipeline 类负责 Vulkan 图形管线的创建和管理，
		涵盖从读取着色器代码到创建图形管线的主要步骤，
		同时提供了可配置和默认的管线设置。
	*/
	LVEPipeline::LVEPipeline(
		LVEDevice& device,
		const std::string& vertFilePath,
		const std::string& fragFilePath,
		const PipelineConfigInfo& configInfo) :lveDevice(device) {
		createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
	}

	LVEPipeline::~LVEPipeline() {
		//销毁着色器模块和图形管线，确保没有内存泄漏。
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

		auto& bindingDescriptions = configInfo.bindingDescriptions;
		auto& attributeDescriptions = configInfo.attributeDescriptions;

		//配置顶点输入状态,保存顶点属性和绑定描述的数量。
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

		//配置图形管线的各种状态。
		//pipelineInfo 包含图形管线的所有创建信息，例如着色器阶段、顶点输入状态、绘制方式（输入装配）、视口状态、光栅化状态、多重采样状态、颜色混合状态、深度模板状态和动态状态等。
		//使用 configInfo 中传入的各种信息来填充图形管线的信息结构。
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
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

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

	//在给定的命令缓冲区中绑定图形管线。
	void LVEPipeline::bind(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	// 默认固定功能状态（生疏时逐字段对照 docs/CODE_GUIDE.md §5.8）
	void LVEPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
		// --- 输入装配：顶点如何拼成图元 ---
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  // 每 3 顶点一三角
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;  // 不用 strip 重启索引

		// --- 视口：个数声明；实际矩形因动态状态每帧 vkCmdSet* ---
		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		// --- 光栅化：三角形 → 片元 ---
		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;       // 不把超近/超远深度夹到 [0,1]
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // FALSE=要真正光栅化
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL; // FILL/线框/点
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;       // 不剔背面（教学省事）
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;// 顺时针为正面
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;         // 阴影 acne 等才开
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

		// --- 多重采样：本工程关闭 MSAA ---
		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;
		configInfo.multisampleInfo.pSampleMask = nullptr;
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

		// --- 颜色混合：默认关闭 = 新片元直接覆盖 ---
		configInfo.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;  // RGBA 都可写
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

		// --- 深度：近小远大，LESS 留下更近的片元 ---
		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};
		configInfo.depthStencilInfo.back = {};

		// --- 动态状态：viewport/scissor 不进管线快照，resize 不必重建管线 ---
		configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;

		// 默认挂上网格顶点布局（点光系统会清空这两项）
		configInfo.bindingDescriptions = LVEModel::Vertex::getBindingDescriptions();
		configInfo.attributeDescriptions = LVEModel::Vertex::getAttributeDescriptions();
	}

	void LVEPipeline::enableAlphaBlending(PipelineConfigInfo& configInfo) {
		configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
		configInfo.colorBlendAttachment.colorWriteMask =											//指定了哪些颜色分量可以被写入到帧缓冲中。
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			//设置源颜色的混合因子
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	//设置目标颜色的混合因子
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;								//源颜色和目标颜色直接相加
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;					//源 Alpha 混合因子设置为 VK_BLEND_FACTOR_ONE，意味着源 Alpha 值完全参与混合。
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;					//目标 Alpha 混合因子设为 VK_BLEND_FACTOR_ZERO，表示目标 Alpha 值不影响混合结果。
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;								//设置 Alpha 混合操作为 VK_BLEND_OP_ADD，同样是加法操作
	}
}  // namespace lve
