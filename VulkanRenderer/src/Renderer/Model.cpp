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

Model::Model(std::string fol, std::string fileName, const VkCommandPool& commandPool)
{
	folder = fol;
	file = fileName;
	loadModel(folder, file, commandPool);
}


Model::~Model()
{
	for (Material mat : materials)
		mat.destroyTextures();

	for (Mesh mesh : meshes)
		mesh.destroyBuffers();
}

void Model::loadModel(std::string fol, std::string fileName, const VkCommandPool& commandPool)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> mats;
	std::string warn, err;
	std::vector<std::string> textureNames;

	// load in vertex and material data
	if (tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err, (DIRECTORY + fol + fileName).c_str(), (DIRECTORY + fol).c_str()))
	{
		std::unordered_map<ModelVertex, uint32_t> uniqueVertices{};

		for (tinyobj::shape_t shape : shapes)
		{
			Mesh newMesh;
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
					uniqueVertices[newVertex] = static_cast<uint32_t>(newMesh.vertices.size());
					newMesh.vertices.push_back(newVertex);
				}				
				newMesh.indices.push_back(uniqueVertices[newVertex]);
			}

			newMesh.material_ID = shape.mesh.material_ids[0]; // figure out how to distribute IDs across faces on mesh

			createVertexBuffer(newMesh, commandPool);
			createIndexBuffer(newMesh, commandPool);
			meshes.push_back(newMesh);
		}

		// load in material
		for (tinyobj::material_t mat : mats)
		{
			Material material;

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
			
			// load in textures
			{
				if (mat.diffuse_texname != "")
				{
					Texture newTex;
					newTex.name = mat.diffuse_texname;
					loadTexture(newTex, commandPool);
					material.diffuseTex = newTex;
					material.createTextureImageView(material.diffuseTex.value(), VK_FORMAT_R8G8B8A8_SRGB);
					material.createTextureSampler(material.diffuseTex.value());
				}

				if (mat.normal_texname != "")
				{
					Texture newTex;
					newTex.name = mat.diffuse_texname;
					loadTexture(newTex, commandPool);
					material.normalTex = newTex;
					material.createTextureImageView(material.normalTex.value(), VK_FORMAT_R8G8B8A8_SRGB);
					material.createTextureSampler(material.normalTex.value());
				}

				if (mat.alpha_texname != "")
				{
					Texture newTex;
					newTex.name = mat.diffuse_texname;
					loadTexture(newTex, commandPool);
					material.alphaTex = newTex;
					material.createTextureImageView(material.alphaTex.value(), VK_FORMAT_R8G8B8A8_SRGB);
					material.createTextureSampler(material.alphaTex.value());
				}

				if (mat.metallic_texname != "")
				{
					Texture newTex;
					newTex.name = mat.diffuse_texname;
					loadTexture(newTex, commandPool);
					material.metallicTex = newTex;
					material.createTextureImageView(material.metallicTex.value(), VK_FORMAT_R8G8B8A8_SRGB);
					material.createTextureSampler(material.metallicTex.value());
				}

				if (mat.displacement_texname != "")
				{
					Texture newTex;
					newTex.name = mat.diffuse_texname;
					loadTexture(newTex, commandPool);
					material.displacementTex = newTex;
					material.createTextureImageView(material.displacementTex.value(), VK_FORMAT_R8G8B8A8_SRGB);
					material.createTextureSampler(material.displacementTex.value());
				}

				if (mat.emissive_texname != "")
				{
					Texture newTex;
					newTex.name = mat.diffuse_texname;
					loadTexture(newTex, commandPool);
					material.emissiveTex = newTex;
					material.createTextureImageView(material.emissiveTex.value(), VK_FORMAT_R8G8B8A8_SRGB);
					material.createTextureSampler(material.emissiveTex.value());
					material.emission = {
						mat.emission[0],
						mat.emission[1],
						mat.emission[2]
					};
				}
			}

			materials.push_back(material);
		}
	}

	else
		throw std::runtime_error("Failed to load model!");
}

void Model::loadTexture(Texture& texture, const VkCommandPool& commandPool)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	int width, height, channels;
	stbi_uc* pixels = stbi_load((DIRECTORY + folder + texture.name).c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels)
		throw std::runtime_error("Failed to load texture image");

	VkDeviceSize imageSize = width * height * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	createImages(width, height, 1, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image, texture.imageMemory);

	transitionImageLayout(commandPool, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(commandPool, stagingBuffer, texture.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);
	transitionImageLayout(commandPool, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

}

void Model::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
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

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = typeIndex;

	// for future reference: THIS IS BAD TO DO FOR INDIVIDUAL BUFFERS (for larger applications)
	// max number of simultaneous memory allocations is limited by maxMemoryAllocationCount in physicalDevice
	// best practice is to create a custom allocator that splits up single allocations among many different objects using the offset param
	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory");

	vkBindBufferMemory(device, buffer, bufferMemory, 0); // if offset is not 0, it MUST be visible by memRequirements.alignment

}

void Model::copyBuffer(const VkCommandPool& pool, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t size, VkQueue queue)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

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

	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
}

void Model::createImages(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

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

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image!");

	// find memory type
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

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

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory");

	vkBindImageMemory(device, image, imageMemory, 0);
}


void Model::transitionImageLayout(const VkCommandPool& commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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

	VkPipelineStageFlags srcStage, dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Model::copyBufferToImage(const VkCommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

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

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Model::createVertexBuffer(Mesh& mesh, const VkCommandPool& commandPool)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkDeviceSize bufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh.vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertexBuffer, mesh.vertexBufferMemory);

	copyBuffer(commandPool, stagingBuffer, mesh.vertexBuffer, bufferSize, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Model::createIndexBuffer(Mesh& mesh, const VkCommandPool& commandPool)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkDeviceSize bufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh.indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer, mesh.indexBufferMemory);

	copyBuffer(commandPool, stagingBuffer, mesh.indexBuffer, bufferSize, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}