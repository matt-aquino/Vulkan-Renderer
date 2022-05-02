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
	// commands
	VkCommandBuffer beginSingleTimeCommands(const VkCommandPool& commandPool);
	void endSingleTimeCommands(const VkCommandPool& commandPool);

	// buffers
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(const VkCommandPool& commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue);
	void copyBufferToImage(const VkCommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

	// images
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, const VkCommandPool& commandPool);
	void createImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, 
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);
	void createImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType);
	void createSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addrMode, VkBool32 enableAnisotropy, float maxAnisotropy, float minLod, float maxLod);

	// pipeline
	VkShaderModule CreateShaderModules(const std::vector<char>& code);
	std::vector<char> readShaderFile(const std::string& file);
}
