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

#include "VulkanDevice.h"

VulkanDevice* VulkanDevice::device = nullptr;
VkPhysicalDevice VulkanDevice::physicalDevice = VK_NULL_HANDLE;
VkDevice VulkanDevice::logicalDevice = VK_NULL_HANDLE;
VulkanDevice::Queue VulkanDevice::queues = { VK_NULL_HANDLE, VK_NULL_HANDLE };
VulkanDevice::QueueFamilyIndices VulkanDevice::familyIndices = {std::nullopt, std::nullopt};
std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VulkanDevice::VulkanDevice()
{
    physicalDevice = VK_NULL_HANDLE;
    logicalDevice = VK_NULL_HANDLE;
    queues.presentQueue = VK_NULL_HANDLE;
    queues.renderQueue = VK_NULL_HANDLE;
}

VulkanDevice::~VulkanDevice()
{
    vkDestroyDevice(logicalDevice, NULL);
    delete device;
}

void VulkanDevice::CreateVulkanDevice(VkInstance appInstance, VkSurfaceKHR appSurface)
{
    device = new VulkanDevice();

    // find suitable physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(appInstance, &deviceCount, NULL);
    if (deviceCount == 0)
        throw std::runtime_error("Failed to find suitable GPU with Vulkan Support!");

    std::vector<VkPhysicalDevice> possibleDevices(deviceCount);
    vkEnumeratePhysicalDevices(appInstance, &deviceCount, possibleDevices.data());
    physicalDevice = possibleDevices[0]; // designed for my computer, which only has 1 GPU

    // grab device properties
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, appSurface);

    float priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    createInfo.enabledLayerCount = 0;
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &queues.renderQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &queues.presentQueue);
}

VulkanDevice* VulkanDevice::GetVulkanDevice()
{
    if (device != nullptr)
        return device;
}

bool VulkanDevice::checkDeviceSupportedExtensions(VkPhysicalDevice dev)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    for (VkExtensionProperties ext : availableExtensions)
    {
        requiredExtensions.erase(ext.extensionName);
    }

    return requiredExtensions.empty();
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface)
{
    // find render and presentation queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    VkBool32 presentSupported = false;

    for (VkQueueFamilyProperties queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            familyIndices.graphicsFamily = i;

        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupported);
        if (presentSupported)
            familyIndices.presentFamily = i;

        if (familyIndices.isComplete())
            break;


        i++;
    }

    return familyIndices;
}
