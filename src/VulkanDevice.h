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

#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <optional>

// VulkanDevice follows the Singleton pattern, which means to only allow one instance to be created
// at runtime. All classes that include VulkanDevice may call upon GetVulkanDevice to retrieve the 
// instance when needed.

class VulkanDevice
{
public:

	static VulkanDevice* GetVulkanDevice();

	VulkanDevice(VulkanDevice &other) = delete; // prevent cloning 
	void operator=(const VulkanDevice&) = delete;

	VkPhysicalDevice GetPhysicalDevice() { return physicalDevice; }
	VkDevice GetLogicalDevice() { return logicalDevice; }
	

private:

	// since the Renderer class performs the main app loop, I decided to make it a friend
	// of VulkanDevice so that it may invoke CreateVulkanDevice at startup
	friend class Renderer;
	void CreateVulkanDevice(VkInstance appInstance, VkSurfaceKHR appSurface);

	VulkanDevice();
	~VulkanDevice();

	// helper functions 
	bool checkDeviceSupportedExtensions(VkPhysicalDevice dev);

	
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	} familyIndices;

	struct Queue
	{
		VkQueue renderQueue;
		VkQueue presentQueue;
	} queues;

	QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface);

public:
	QueueFamilyIndices GetFamilyIndices() { return familyIndices; }
	Queue GetQueues() { return queues; }

protected:

	// device instance
	static VulkanDevice* device;

	
};

#endif //!VULKANDEVICE_H