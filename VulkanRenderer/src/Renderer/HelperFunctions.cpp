#include "HelperFunctions.h"
#include <fstream>
#include "HelperStructs.h"
#include <vulkan/vulkan.h>

namespace HelperFunctions
{
	namespace initializers
	{
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 restart)
		{
			VkPipelineInputAssemblyStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
			createInfo.flags = flags;
			createInfo.topology = topology;
			createInfo.primitiveRestartEnable = restart;
			return createInfo;
		}

		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(int numBindingDescriptions, VkVertexInputBindingDescription& bindingDescriptions, int numAttributeDescriptions, VkVertexInputAttributeDescription* attributeDescriptions)
		{
			VkPipelineVertexInputStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
			createInfo.vertexBindingDescriptionCount = numBindingDescriptions;
			createInfo.pVertexBindingDescriptions = &bindingDescriptions;
			createInfo.vertexAttributeDescriptionCount = numAttributeDescriptions;
			createInfo.pVertexAttributeDescriptions = attributeDescriptions;
			return createInfo;
		}

		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace front)
		{
			VkPipelineRasterizationStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
			createInfo.polygonMode = polygonMode;
			createInfo.cullMode = cullMode;
			createInfo.frontFace = front;
			return createInfo;
		}

		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(int numAttachments, VkPipelineColorBlendAttachmentState blendAttachState)
		{
			VkPipelineColorBlendStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
			createInfo.attachmentCount = numAttachments;
			createInfo.pAttachments = &blendAttachState;
			return createInfo;
		}

		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(VkBool32 enableDepthTest, VkBool32 enableDepthWrite, VkCompareOp compareOp)
		{
			VkPipelineDepthStencilStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
			createInfo.depthTestEnable = enableDepthTest;
			createInfo.depthWriteEnable = enableDepthWrite;
			createInfo.depthCompareOp = compareOp;
			return createInfo;
		}

		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(int viewportCount, int scissorCount, VkPipelineViewportStateCreateFlags flags)
		{
			VkPipelineViewportStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
			createInfo.viewportCount = viewportCount;
			createInfo.scissorCount = scissorCount;
			createInfo.flags = flags;
			return createInfo;
		}

		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(VkSampleCountFlagBits flags)
		{
			VkPipelineMultisampleStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
			createInfo.rasterizationSamples = flags;
			return createInfo;
		}

		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(int numDynamicStates, VkDynamicState* dynamicStates)
		{
			VkPipelineDynamicStateCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
			createInfo.dynamicStateCount = numDynamicStates;
			createInfo.pDynamicStates = dynamicStates;
			return createInfo;
		}

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stageFlags, VkShaderModule shaderModule)
		{
			VkPipelineShaderStageCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			createInfo.stage = stageFlags;
			createInfo.module = shaderModule;
			createInfo.pName = "main";
			return createInfo;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(int numSetLayouts, VkDescriptorSetLayout* setLayouts, int numPushRanges, VkPushConstantRange* pushRange)
		{
			VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
			createInfo.pushConstantRangeCount = numPushRanges;
			createInfo.pPushConstantRanges = pushRange;
			createInfo.setLayoutCount = numSetLayouts;
			createInfo.pSetLayouts = setLayouts;
			return createInfo;
		}

		VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet& dstSet, const VkDescriptorBufferInfo* bufferInfo, uint32_t dstBinding, VkDescriptorType bufferType, const VkBufferView* bufferView)
		{
			VkWriteDescriptorSet writeDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.descriptorType = bufferType;
			writeDescriptor.dstSet = dstSet;
			writeDescriptor.dstBinding = dstBinding;
			writeDescriptor.pBufferInfo = bufferInfo;
			writeDescriptor.pTexelBufferView = bufferView;
			return writeDescriptor;
		}
		VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet& dstSet, const VkDescriptorImageInfo* imageInfo, uint32_t dstBinding)
		{
			VkWriteDescriptorSet writeDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptor.dstSet = dstSet;
			writeDescriptor.dstBinding = dstBinding;
			writeDescriptor.pImageInfo = imageInfo;
			return writeDescriptor;
		}
	}

	VkCommandBuffer beginSingleTimeCommands(const VkCommandPool& commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkQueue queue, const VkCommandPool& commandPool)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);
		vkFreeCommandBuffers(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), commandPool, 1, &cmdBuffer);
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can be shared between queue families just like images

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to create vertex buffer");

		// assign memory to buffer
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(VulkanDevice::GetVulkanDevice()->GetPhysicalDevice(), &memProperties);

		uint32_t typeIndex = 0;
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				typeIndex = i;
				break;
			}
		}

		VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = typeIndex;

		// for future reference: THIS IS BAD TO DO FOR INDIVIDUAL BUFFERS (for larger applications)
		// max number of simultaneous memory allocations is limited by maxMemoryAllocationCount in physicalDevice
		// best practice is to create a custom allocator that splits up single allocations among many different objects using the offset param
		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate buffer memory");

		vkBindBufferMemory(device, buffer, bufferMemory, 0); // if offset is not 0, it MUST be visible by memRequirements.alignment

	}

	void copyBuffer(const VkCommandPool& commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer, queue, commandPool);
	}

	void copyBufferToImage(const VkCommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, depth };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue, commandPool);
	}

	VkShaderModule CreateShaderModules(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module");

		return shaderModule;
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, 
		const VkCommandPool& commandPool, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
	{
		VulkanDevice* vkDevice = VulkanDevice::GetVulkanDevice();
		VkDevice device = vkDevice->GetLogicalDevice();
		VkQueue renderQueue = vkDevice->GetQueues().renderQueue;

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		switch (oldLayout)
			{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					barrier.srcAccessMask = 0;
					break;

				case VK_IMAGE_LAYOUT_PREINITIALIZED:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image is a transfer source
					// Make sure any reads from the image have been finished
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					// Other source layouts aren't handled (yet)
					throw std::invalid_argument("Unsupported layout transition!");
					break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
		switch (newLayout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (barrier.srcAccessMask == 0)
				{
					barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				throw std::invalid_argument("Unsupported layout transition!");
				break;
		}

		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		endSingleTimeCommands(commandBuffer, renderQueue, commandPool);
	}

	void createImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, 
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory)
	{
		static VkPhysicalDevice physical = VulkanDevice::GetVulkanDevice()->GetPhysicalDevice();
		static VkDevice logical = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = imageType;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = depth;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(logical, &imageInfo, nullptr, &image) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image!");

		// find memory type
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logical, image, &memRequirements);

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physical, &memProperties);

		uint32_t typeIndex = 0;
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				typeIndex = i;
				break;
			}
		}

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = typeIndex;

		if (vkAllocateMemory(logical, &allocInfo, nullptr, &memory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate image memory");

		if (vkBindImageMemory(logical, image, memory, 0) != VK_SUCCESS)
			throw std::runtime_error("Failed to bind image memory");

	}

	void createImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType)
	{
		VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = image;
		viewInfo.viewType = viewType;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view");
	}

	void createSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addrMode, VkBool32 enableAnisotropy, float maxAnisotropy, float minLod, float maxLod, VkBorderColor borderColor)
	{
		static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

		VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
		samplerInfo.addressModeU = addrMode;
		samplerInfo.addressModeV = addrMode;
		samplerInfo.addressModeW = addrMode;
		samplerInfo.anisotropyEnable = enableAnisotropy;
		samplerInfo.maxAnisotropy = maxAnisotropy;
		samplerInfo.borderColor = borderColor;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = minLod;
		samplerInfo.maxLod = maxLod;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler");
	}

	std::vector<char> readShaderFile(const std::string& file)
	{
		std::ifstream fileStream(file, std::ios::ate | std::ios::binary);

		if (!fileStream.is_open())
		{
			throw std::runtime_error("Failed to open shader file!");
		}

		size_t shaderFileSize = (size_t)fileStream.tellg();
		std::vector<char> buffer(shaderFileSize);

		fileStream.seekg(0);
		fileStream.read(buffer.data(), shaderFileSize);

		fileStream.close();
		return buffer;
	}

	
}