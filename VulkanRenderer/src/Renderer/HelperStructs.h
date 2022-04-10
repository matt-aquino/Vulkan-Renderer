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
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include "VulkanDevice.h"

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

struct VulkanBuffer
{
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
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

	// for scenes where vertex data is passed in raw, instead of stored in a mesh
	std::optional<VulkanBuffer> vertexBuffer; 
	std::optional<VulkanBuffer> indexBuffer;

	std::vector<VulkanBuffer> uniformBuffers;

	bool isDepthBufferEmpty = true,
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
			vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
			vkFreeMemory(device, uniformBuffers[i].bufferMemory, nullptr);
		}

		// check is buffers are filled before attempting to delete them
		if (vertexBuffer.has_value())
		{
			vkDestroyBuffer(device, vertexBuffer->buffer, nullptr);
			vkFreeMemory(device, vertexBuffer->bufferMemory, nullptr);
		}

		if (indexBuffer.has_value())
		{
			vkDestroyBuffer(device, indexBuffer->buffer, nullptr);
			vkFreeMemory(device, indexBuffer->bufferMemory, nullptr);
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

struct Texture
{
	std::string name;
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkSampler sampler;
};

struct Material
{
	alignas(16)glm::vec3 ambient{};
	alignas(16)glm::vec3 diffuse{};
	alignas(16)glm::vec3 specular{};
	alignas(4)float specularExponent = 0.0f;

	std::optional<Texture> diffuseTex;
	std::optional<Texture> normalTex;
	std::optional<Texture> alphaTex;
	std::optional<Texture> metallicTex;
	std::optional<Texture> displacementTex;
	std::optional<Texture> emissiveTex;
	std::optional<glm::vec3> emission;

	void destroyTextures()
	{
		VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		// destroy each texture if it holds a value
		{
			if (diffuseTex.has_value())
			{
				vkDestroyImage(device, diffuseTex->image, nullptr);
				vkDestroyImageView(device, diffuseTex->imageView, nullptr);
				vkDestroySampler(device, diffuseTex->sampler, nullptr);
				vkFreeMemory(device, diffuseTex->imageMemory, nullptr);
			}

			if (normalTex.has_value())
			{
				vkDestroyImage(device, normalTex->image, nullptr);
				vkDestroyImageView(device, normalTex->imageView, nullptr);
				vkDestroySampler(device, normalTex->sampler, nullptr);
				vkFreeMemory(device, normalTex->imageMemory, nullptr);
			}

			if (alphaTex.has_value())
			{
				vkDestroyImage(device, alphaTex->image, nullptr);
				vkDestroyImageView(device, alphaTex->imageView, nullptr);
				vkDestroySampler(device, alphaTex->sampler, nullptr);
				vkFreeMemory(device, alphaTex->imageMemory, nullptr);
			}

			if (metallicTex.has_value())
			{
				vkDestroyImage(device, metallicTex->image, nullptr);
				vkDestroyImageView(device, metallicTex->imageView, nullptr);
				vkDestroySampler(device, metallicTex->sampler, nullptr);
				vkFreeMemory(device, metallicTex->imageMemory, nullptr);
			}

			if (displacementTex.has_value())
			{
				vkDestroyImage(device, displacementTex->image, nullptr);
				vkDestroyImageView(device, displacementTex->imageView, nullptr);
				vkDestroySampler(device, displacementTex->sampler, nullptr);
				vkFreeMemory(device, displacementTex->imageMemory, nullptr);
			}

			if (emissiveTex.has_value())
			{
				vkDestroyImage(device, emissiveTex->image, nullptr);
				vkDestroyImageView(device, emissiveTex->imageView, nullptr);
				vkDestroySampler(device, emissiveTex->sampler, nullptr);
				vkFreeMemory(device, emissiveTex->imageMemory, nullptr);
			}
		}
		
	}

	void createTextureImageView(Texture& texture, VkFormat format)
	{
		static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = texture.image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view");
	}

	void createTextureSampler(Texture& texture)
	{
		VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 4.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler");
	}
};

struct Mesh
{
	std::vector<ModelVertex> vertices;
	std::vector<uint32_t> indices;
	VulkanBuffer vertexBuffer, indexBuffer;
	int material_ID;

	void destroyBuffers()
	{
		VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
		vkDestroyBuffer(device, vertexBuffer.buffer, nullptr);
		vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
		vkFreeMemory(device, vertexBuffer.bufferMemory, nullptr);
		vkFreeMemory(device, indexBuffer.bufferMemory, nullptr);
	}
};

#endif //!HELPERSTRUCTS_H