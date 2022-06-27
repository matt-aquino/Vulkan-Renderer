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

#pragma once
#include <vulkan/vulkan.h>
#include "VulkanDevice.h"
#include <string>

namespace HelperFunctions
{
	namespace initializers
	{
		// defines primitive topology 
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkPipelineInputAssemblyStateCreateFlags flags = 0, VkBool32 restart = VK_FALSE);
		
		// use when you need an empty vertex state: e.g rendering a full screen quad
		VkPipelineVertexInputStateCreateInfo   pipelineVertexInputStateCreateInfo();

		// use for every other case of vertex input
		VkPipelineVertexInputStateCreateInfo   pipelineVertexInputStateCreateInfo(int numBindingDescriptions, VkVertexInputBindingDescription& bindingDescriptions, int numAttributeDescriptions, VkVertexInputAttributeDescription* attributeDescriptions);
		
		// define rasterization state, such as fill mode, which faces to cull, and vertex order
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL, VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT, VkFrontFace front = VK_FRONT_FACE_COUNTER_CLOCKWISE);
		
		// define attachments for color blending
		VkPipelineColorBlendStateCreateInfo    pipelineColorBlendStateCreateInfo(int numAttachments, VkPipelineColorBlendAttachmentState& blendAttachState);
		
		// define depth and stencil state
		VkPipelineDepthStencilStateCreateInfo  pipelineDepthStencilStateCreateInfo(VkBool32 enableDepthTest = VK_TRUE, VkBool32 enableDepthWrite = VK_TRUE, VkCompareOp compareOp = VK_COMPARE_OP_LESS);
		VkPipelineViewportStateCreateInfo	   pipelineViewportStateCreateInfo(int viewportCount, int scissorCount, VkPipelineViewportStateCreateFlags flags);
		VkPipelineMultisampleStateCreateInfo   pipelineMultisampleStateCreateInfo(VkSampleCountFlagBits flags = VK_SAMPLE_COUNT_1_BIT);
		VkPipelineDynamicStateCreateInfo	   pipelineDynamicStateCreateInfo(int numDynamicStates, VkDynamicState* dynamicStates);
		VkPipelineShaderStageCreateInfo		   pipelineShaderStageCreateInfo(VkShaderStageFlagBits stageFlags, VkShaderModule shaderModule);
		VkPipelineLayoutCreateInfo			   pipelineLayoutCreateInfo(int numSetLayouts, VkDescriptorSetLayout* setLayouts, int numPushRanges = 0, VkPushConstantRange* pushRange = nullptr);


		VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet& dstSet, const VkDescriptorBufferInfo* bufferInfo, uint32_t dstBinding = 0, VkDescriptorType bufferType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, const VkBufferView* bufferView = nullptr);
		VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet& dstSet, const VkDescriptorImageInfo* imageInfo, uint32_t dstBinding = 0);
	}

	// commands
	VkCommandBuffer beginSingleTimeCommands(const VkCommandPool& commandPool);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkQueue queue, const VkCommandPool& commandPool);

	// buffers
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(const VkCommandPool& commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue);
	void copyBufferToImage(const VkCommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

	// images
	void transitionImageLayout(VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout, const VkCommandPool& commandPool, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
	void createImage(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, VkSampleCountFlagBits sampleCount, VkImageType imageType, VkFormat format, VkImageTiling tiling, 
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);
	void createImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t mipLevels);
	void createSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addrMode, VkBool32 enableAnisotropy = VK_FALSE, float maxAnisotropy = 0.0f, float minLod = 0.0f, int32_t mipLevels = 1, VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
	void generateImageMipmaps(VkImage image, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, const VkCommandPool& commandPool);

	// pipeline
	VkShaderModule CreateShaderModules(const std::vector<char>& code);
	std::vector<char> readShaderFile(const std::string& file);

	VkSampleCountFlagBits getMaximumSampleCount();
}
