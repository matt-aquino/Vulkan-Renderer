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
#include <tiny_obj_loader.h>
#include <string>
#include <vector>
#include "HelperStructs.h"
#include "VulkanDevice.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MODELPATH "../models/"

class Model
{
public:
	Model(std::string fileName, std::string imageName);
	~Model();

	

private:
	void loadModel(std::string fileName);
	void loadTexture(std::string imageName);


	// ** Allocate a buffer of memory based on specifications **
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	
	
	void createImages(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	struct Material
	{
		VkImage texture;
		VkDeviceMemory textureMemory;

		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		float specularExponent;
	} material;

	VkBuffer vertexBuffer, indexBuffer;
	VkDeviceMemory vertexBufferMemory, indexBufferMemory;
};


#endif //!MODEL_H