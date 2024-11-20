#include "lve_pipeline.h"

#include <fstream>
#include <stdexcept>
#include <iostream>

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
		auto vectCode = readFile(vertFilePath);
		auto fragCode = readFile(fragFilePath);

		std::cout << "vertex shader code size: " << vectCode.size() << std::endl;
		std::cout << "vertex shader code size: " << vectCode.size() << std::endl;
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

	PipelineConfigInfo LVEPipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
		PipelineConfigInfo configInfo{};
		return configInfo;
	}

}