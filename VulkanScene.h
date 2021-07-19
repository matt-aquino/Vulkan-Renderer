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
#include <array>

#define SHADERPATH "Shaders/"


class VulkanScene
{
public:
	VulkanScene();
	~VulkanScene();

	virtual void CreateScene() = 0;
	virtual void RecreateScene(const VulkanSwapChain& swapChain) = 0;
	virtual VulkanReturnValues RunScene(const VulkanSwapChain& swapChain) = 0;
	virtual void DestroyScene() = 0;

	std::string sceneName;
protected:


	VulkanDevice *device;

	// shaders
	virtual void CreateShaderModules(const std::vector<char>& code) = 0;
	virtual void CreateCommandPool() = 0;

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue);


	// Commands
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffersList;

	// Synchronzation
	std::vector<VkSemaphore> renderCompleteSemaphores, presentCompleteSemaphores;
	std::vector<VkFence> inFlightFences, imagesInFlight;


};

#endif // !VULKANSCENE_H