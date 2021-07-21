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

#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Model::Model(std::string dir, std::string fileName, std::string imageName)
{
	loadModel(dir, fileName);
	loadTexture(dir + imageName);
}


Model::~Model()
{
	vkDestroyBuffer(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), vertexBuffer, nullptr);
	vkDestroyBuffer(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), indexBuffer, nullptr);
	vkDestroyImage(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), material.texture, nullptr);
	vkFreeMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), vertexBufferMemory, nullptr);
	vkFreeMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), indexBufferMemory, nullptr);
	vkFreeMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), material.textureMemory, nullptr);
}

void Model::loadModel(std::string dir, std::string fileName)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, ("models/" + dir + fileName).c_str(), ("models/" + dir).c_str()))
	{
		std::unordered_map<ModelVertex, uint32_t> uniqueVertices{};

		for (tinyobj::shape_t shape : shapes)
		{
			for (tinyobj::index_t index : shape.mesh.indices)
			{
				ModelVertex newVertex{};
				newVertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				newVertex.texcoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Vulkan Y-axis is flipped, so make sure we adjust textures
				};

				newVertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				// check if we've already seen this vertex or not
				if (uniqueVertices.count(newVertex) == 0)
				{
					uniqueVertices[newVertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(newVertex);
				}

				indices.push_back(uniqueVertices[newVertex]);
			}
		}

		// load in material
		for (tinyobj::material_t mat : materials)
		{
			material.ambient = {
				mat.ambient[0],
				mat.ambient[1],
				mat.ambient[2],
			};

			material.diffuse = {
				mat.diffuse[0],
				mat.diffuse[1],
				mat.diffuse[2]
			};

			material.specular = {
				mat.specular[0],
				mat.specular[1],
				mat.specular[2]
			};

			material.specularExponent = mat.shininess;
		}
	}

	else
		throw std::runtime_error("Failed to load model!");
	
}

void Model::loadTexture(std::string imageName)
{
	int width, height, channels;
	stbi_uc* pixels = stbi_load(("models/" + imageName).c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels)
		throw std::runtime_error("Failed to load texture image");

	VkDeviceSize imageSize = width * height * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	createImages(width, height, 1, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, material.texture, material.textureMemory);

	vkDestroyBuffer(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory, nullptr);
}


void Model::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can be shared between queue families just like images

	if (vkCreateBuffer(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer");

	// assign memory to buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), buffer, &memRequirements);

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

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = typeIndex;

	// for future reference: THIS IS BAD TO DO FOR INDIVIDUAL BUFFERS (for larger applications)
	// max number of simultaneous memory allocations is limited by maxMemoryAllocationCount in physicalDevice
	// best practice is to create a custom allocator that splits up single allocations among many different objects using the offset param
	if (vkAllocateMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory");

	vkBindBufferMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), buffer, bufferMemory, 0); // if offset is not 0, it MUST be visible by memRequirements.alignment

}

void Model::copyBuffer(const VkCommandPool& pool, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t size, VkQueue queue)
{
	VkCommandBuffer commandBuffer;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkAllocateCommandBuffers(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &allocInfo, &commandBuffer);

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	vkFreeCommandBuffers(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), pool, 1, &commandBuffer);
}

void Model::createImages(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = imageType;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image!");

	// find memory type
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), image, &memRequirements);

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

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = typeIndex;

	if (vkAllocateMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory");

	vkBindImageMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), image, imageMemory, 0);
}

void Model::createVertexBuffer(const VkCommandPool& commandPool)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	copyBuffer(commandPool, stagingBuffer, vertexBuffer, bufferSize, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	vkDestroyBuffer(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void Model::createIndexBuffer(const VkCommandPool& commandPool)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(commandPool, stagingBuffer, indexBuffer, bufferSize, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	vkDestroyBuffer(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), stagingBufferMemory, nullptr);
}



std::vector<ModelVertex> Model::getVertices()
{
	return vertices;
}

std::vector<uint32_t> Model::getIndices()
{
	return indices;
}

VkBuffer Model::getVertexBuffer()
{
	return vertexBuffer;
}

VkBuffer Model::getIndexBuffer()
{
	return indexBuffer;
}