#include "lve_pipeline.h"

#include <fstream>
#include <stdexcept>
#include <iostream>

namespace lve {
	LVEPipeline::LVEPipeline(const std::string& vertFilePath, const std::string& fragFilePath) {
		createGraphicsPipeline(vertFilePath, fragFilePath);
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

	void LVEPipeline::createGraphicsPipeline(const std::string& vertFilePath, const std::string& fragFilePath) {
		auto vectCode = readFile(vertFilePath);
		auto fragCode = readFile(fragFilePath);

		std::cout << "vertex shader code size: " << vectCode.size() << std::endl;
		std::cout << "vertex shader code size: " << vectCode.size() << std::endl;
	}
}