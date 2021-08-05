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

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

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

struct VulkanGraphicsPipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorPool descriptorPool;

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	VkImage depthStencilBuffer;
	VkImageView depthStencilBufferView;
	VkDeviceMemory depthStencilBufferMemory;

	VkViewport viewport;
	VkRect2D scissors;

	VkBuffer vertexBuffer, indexBuffer;
	VkDeviceMemory vertexBufferMemory, indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	bool isVertexBufferEmpty = true,
		 isIndexBufferEmpty = true,
		 isDepthBufferEmpty = true,
		 isDescriptorPoolEmpty = true;

	size_t shaderFileSize;

	VkResult result;

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

	void destroyGraphicsPipeline(const VkDevice& device)
	{
		for (VkFramebuffer fb : framebuffers)
		{
			vkDestroyFramebuffer(device, fb, nullptr);
		}

		size_t uniformBufferSize = uniformBuffers.size();
		for (size_t i = 0; i < uniformBufferSize; i++)
		{
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		// check is buffers are filled before attempting to delete them
		if (!isVertexBufferEmpty)
		{
			vkDestroyBuffer(device, vertexBuffer, nullptr);
			vkFreeMemory(device, vertexBufferMemory, nullptr);
		}
		if (!isIndexBufferEmpty)
		{
			vkDestroyBuffer(device, indexBuffer, nullptr);
			vkFreeMemory(device, indexBufferMemory, nullptr);
		}
		if (!isDepthBufferEmpty)
		{
			vkDestroyImageView(device, depthStencilBufferView, nullptr);
			vkDestroyImage(device, depthStencilBuffer, nullptr);
			vkFreeMemory(device, depthStencilBufferMemory, nullptr);
		}
		
		if (!isDescriptorPoolEmpty)
		{
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		}

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
	}
};

struct VulkanComputePipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkBuffer storageBuffer, uniformBuffer;
	VkDeviceMemory storageBufferMemory, uniformBufferMemory;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	size_t shaderFileSize;
	VkResult result;
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

	void destroyComputePipeline(const VkDevice& device)
	{
		vkDestroyBuffer(device, storageBuffer, nullptr);
		vkFreeMemory(device, storageBufferMemory, nullptr);
		vkDestroyBuffer(device, uniformBuffer, nullptr);
		vkFreeMemory(device, uniformBufferMemory, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	}
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
	alignas(16)glm::vec3 position; 
	alignas(8)glm::vec2 texcoord;
	alignas(16)glm::vec3 normal;

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

	bool operator==(const ModelVertex& other) const
	{
		return position == other.position &&
			normal == other.normal && texcoord == other.texcoord;
	}
};

struct Particle
{
	alignas(16)glm::vec3 position;
	alignas(16)glm::vec3 velocity;
	alignas(16)glm::vec3 color;
	/*
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindDesc = {};
		bindDesc.binding = 0;
		bindDesc.stride = sizeof(Particle);
		bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attrDesc{};
		attrDesc[0].binding = 0;
		attrDesc[0].location = 0; // match the location within the shader
		attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; // match format within shader (float, vec2, vec3, vec4,)
		attrDesc[0].offset = offsetof(Particle, position);

		attrDesc[1].binding = 0;
		attrDesc[1].location = 1;
		attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[1].offset = offsetof(Particle, velocity);

		attrDesc[2].binding = 0;
		attrDesc[2].location = 2;
		attrDesc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[2].offset = offsetof(Particle, color);

		return attrDesc;
	}
	*/
};

namespace std
{
	template<> struct hash<ModelVertex>
	{
		size_t operator()(ModelVertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texcoord) << 1);
		}
	};
}

struct Material
{
	alignas(16)glm::vec3 ambient{};
	alignas(16)glm::vec3 diffuse{};
	alignas(16)glm::vec3 specular{};
	alignas(4)float specularExponent = 0;
};

#endif //!HELPERSTRUCTS_H