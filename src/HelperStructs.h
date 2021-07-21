/*
   Copyright 2021 Matthew Aquino

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef HELPERSTRUCTS_H
#define HELPERSTRUCTS_H

#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>

struct VulkanSwapChain
{
	VkSwapchainKHR swapChain{};
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainDimensions;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> surfacePresentModes;
};

// Ways to Pass uniforms into shaders

// UBOs and SSBOs are for dynamic data
// since we have small amounts of data, we opt to use UBOs
struct UniformBufferObject
{
	glm::mat4 model; 
	glm::mat4 view;
};

// Push Constants are for static data that never change during runtime
struct PushConstants
{
	glm::mat4 projectionMatrix;
};

struct VulkanGraphicsPipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorPool descriptorPool;

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	VkViewport viewport;
	VkRect2D scissors;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;


	size_t shaderFileSize;

	VkResult result;

	UniformBufferObject ubo;
	PushConstants pushConstant;

	std::vector<char> readShaderFile(const std::string& file)
	{
		std::ifstream fileStream(file, std::ios::ate | std::ios::binary);

		if (!fileStream.is_open())
		{
			throw std::runtime_error("Failed to open shader file!");
		}

		shaderFileSize = (size_t)fileStream.tellg();
		std::vector<char> buffer(shaderFileSize);

		fileStream.seekg(0);
		fileStream.read(buffer.data(), shaderFileSize);

		fileStream.close();
		return buffer;
	}
};

struct VulkanComputePipeline
{
	// will need to fill this eventually
};

enum class VulkanReturnValues
{
	VK_SWAPCHAIN_OUT_OF_DATE,
	VK_FUNCTION_SUCCESS,
	VK_FUNCTION_FAILED,
};

struct Vertex
{
	glm::vec3 position; // for 2D objects, simply ignore the z value
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindDesc = {};
		bindDesc.binding = 0;
		bindDesc.stride = sizeof(Vertex);
		bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attrDesc{};
		attrDesc[0].binding = 0;
		attrDesc[0].location = 0; // match the location within the shader
		attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; // match format within shader (float, vec2, vec3, vec4,)
		attrDesc[0].offset = offsetof(Vertex, position);

		attrDesc[1].binding = 0;
		attrDesc[1].location = 1;
		attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[1].offset = offsetof(Vertex, color);

		return attrDesc;
	}
};

struct ModelVertex
{
	glm::vec3 position; 
	glm::vec2 texcoord;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindDesc = {};
		bindDesc.binding = 0;
		bindDesc.stride = sizeof(ModelVertex);
		bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attrDesc{};
		attrDesc[0].binding = 0;
		attrDesc[0].location = 0; // match the location within the shader
		attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; // match format within shader (float, vec2, vec3, vec4,)
		attrDesc[0].offset = offsetof(ModelVertex, position);

		attrDesc[1].binding = 0;
		attrDesc[1].location = 1;
		attrDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
		attrDesc[1].offset = offsetof(ModelVertex, texcoord);
		
		attrDesc[2].binding = 0;
		attrDesc[2].location = 2;
		attrDesc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[2].offset = offsetof(ModelVertex, normal);

		return attrDesc;
	}
};


#endif //!HELPERSTRUCTS_H