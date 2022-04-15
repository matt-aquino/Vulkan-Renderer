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

#ifndef MODEL_H
#define MODEL_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "HelperStructs.h"
#include "VulkanDevice.h"

class Model
{
public:
	Model(std::string folder, std::string objFile, const VkCommandPool& commandPool);
	~Model();
	std::vector<Mesh> meshes;
	std::vector<Material> materials;

private:
	void loadModel(std::string fol, std::string fileName, const VkCommandPool& commmandPool);
	void loadTexture(Texture& texture, const VkCommandPool& commandPool);

	void createVertexBuffer(Mesh& mesh, const VkCommandPool& commandPool);
	void createIndexBuffer(Mesh& mesh, const VkCommandPool& commandPool);

	// ** Allocate a buffer of memory based on specifications **
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	
	void copyBuffer(const VkCommandPool& pool, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t size, VkQueue queue);

	void createImages(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	void createImageViews(VkImage image, VkFormat format, uint32_t index);

	void createTextureSampler(uint32_t index);

	// ** Transition images layouts **
	void transitionImageLayout(const VkCommandPool& commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	
	// ** Copy buffer data into an image **
	void copyBufferToImage(const VkCommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);


	const std::string DIRECTORY = "assets/models/";
	std::string folder, file;
};


#endif //!MODEL_H