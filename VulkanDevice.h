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

class VulkanDevice
{
public:

	// grab device instance; create one if not initialized
	static VulkanDevice* GetVulkanDevice(VkInstance appInstance, VkSurfaceKHR appSurface);

	VulkanDevice(VulkanDevice &other) = delete; // prevent cloning 
	void operator=(const VulkanDevice&) = delete;

	static VkPhysicalDevice physicalDevice;
	static VkDevice logicalDevice;

private:

	VulkanDevice();
	~VulkanDevice();

	// helper functions 
	bool checkDeviceSupportedExtensions(VkPhysicalDevice dev);
	

	static struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	} familyIndices;

	static struct Queue
	{
		VkQueue renderQueue;
		VkQueue presentQueue;
	} queues;

	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);

public:
	inline static QueueFamilyIndices GetFamilyIndices() { return familyIndices; }
	inline static Queue GetQueues() { return queues; }

protected:

	// device instance
	static VulkanDevice* device;

	
};

#endif //!VULKANDEVICE_H