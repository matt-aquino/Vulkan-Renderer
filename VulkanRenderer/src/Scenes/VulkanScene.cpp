#include "VulkanScene.h"
VulkanScene::VulkanScene()
{
	CreateCommandPool();
}

VulkanScene::~VulkanScene()
{
	ModelLoader::destroy();
	TextureLoader::destroy();
	BasicShapes::destroyShapes();
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	vkDestroyCommandPool(device, commandPool, nullptr);
}

void VulkanScene::CreateCommandPool()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = VulkanDevice::GetVulkanDevice()->GetFamilyIndices().graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool");
	
	ModelLoader::setCommandPool(commandPool);
	TextureLoader::setCommandPool(commandPool);
	BasicShapes::loadShapes(commandPool);
}

