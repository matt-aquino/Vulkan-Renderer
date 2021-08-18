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

#ifndef VULKANSCENE_H
#define VULKANSCENE_H

#include <vulkan/vulkan.h>
#include "VulkanDevice.h"
#include "HelperStructs.h"
#include "Camera.h"
#include "Model.h"

#define SHADERPATH "Shaders/"


class VulkanScene
{
public:
	VulkanScene();
	~VulkanScene();

	// ** Perform initial setup of a scene
	virtual void RecordScene() = 0;

	// ** Recreate the scene when swap chain goes out of date **
	virtual void RecreateScene(const VulkanSwapChain& swapChain) = 0;

	// ** perform main loop of scene **
	virtual VulkanReturnValues DrawScene(const VulkanSwapChain& swapChain) = 0;
	
	// ** Clean up resources ** 
	virtual void DestroyScene(bool isRecreation) = 0;

	std::string sceneName;

protected:

	// ** Allocate memory to a command pool for command buffers **
	void CreateCommandPool();

	// ** Read shader files and create the shader modules used in a pipeline **
	VkShaderModule CreateShaderModules(const std::vector<char>& code);

	// ** Allocate a buffer of memory based on specifications **
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	
	// ** Copy an existing buffer into another **
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue);

	// ** Copy buffer data into an image **
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

	// ** Transition images layouts **
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	// ** submit single time commands on command buffers **
	VkCommandBuffer beginSingleTimeCommands();

	// ** end single time commands **
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkQueue queue);

	void createImage(uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);

	void createImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType);


	// Command Buffers
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffersList;

	// Synchronzation Objects
	std::vector<VkSemaphore> renderCompleteSemaphores, presentCompleteSemaphores;
	std::vector<VkFence> inFlightFences, imagesInFlight;
	const int MAX_FRAMES_IN_FLIGHT = 3; // we need 1 fence per frame. since we're triple buffering, that means 3 fences
};

#endif // !VULKANSCENE_H