#pragma once

#include <string>
#include <vector>
#include "lve_device.h"

struct PipelineConfigInfo {};

namespace lve{
	class LVEPipeline {
	public:
		LVEPipeline(
			LVEDevice& device,
			const std::string& vertFilePath,
			const std::string& fragFilePath,
			const PipelineConfigInfo& configInfo);

		~LVEPipeline();

		LVEPipeline(const LVEPipeline&) = delete;
		void operator=(const LVEPipeline&) = delete;

		static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);


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