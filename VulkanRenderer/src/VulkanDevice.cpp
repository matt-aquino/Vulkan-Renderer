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
    delete device;
}

void VulkanDevice::CreateVulkanDevice(VkInstance appInstance, VkSurfaceKHR appSurface)
{
    // find suitable physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(appInstance, &deviceCount, NULL);
    if (deviceCount == 0)
        throw std::runtime_error("Failed to find suitable GPU with Vulkan Support!");

    std::vector<VkPhysicalDevice> possibleDevices(deviceCount);
    vkEnumeratePhysicalDevices(appInstance, &deviceCount, possibleDevices.data());
    device->physicalDevice = possibleDevices[0]; // designed for my computer, which only has 1 GPU

    // grab device properties
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device->physicalDevice, &properties);
    vkGetPhysicalDeviceFeatures(device->physicalDevice, &features);
    features.samplerAnisotropy = VK_TRUE;

    QueueFamilyIndices indices = findQueueFamilies(appSurface);

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
    VkResult result = vkCreateDevice(device->physicalDevice, &createInfo, nullptr, &device->logicalDevice);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    vkGetDeviceQueue(device->logicalDevice, indices.graphicsFamily.value(), 0, &device->queues.renderQueue);
    vkGetDeviceQueue(device->logicalDevice, indices.presentFamily.value(), 0, &device->queues.presentQueue);
}

VulkanDevice* VulkanDevice::GetVulkanDevice()
{
    if (device == nullptr)
        device = new VulkanDevice();

    return device;
}

void VulkanDevice::DeleteLogicalDevice()
{
    vkDestroyDevice(logicalDevice, nullptr);
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

VkFormat VulkanDevice::findSupportedFormats(std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(VkSurfaceKHR surface)
{
    // find render and presentation queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device->physicalDevice, &queueFamilyCount, NULL);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device->physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    VkBool32 presentSupported = false;

    for (VkQueueFamilyProperties queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            device->familyIndices.graphicsFamily = i;

        vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice, i, surface, &presentSupported);
        if (presentSupported)
            device->familyIndices.presentFamily = i;

        if (device->familyIndices.isComplete())
            break;


        i++;
    }

    return device->familyIndices;
}
