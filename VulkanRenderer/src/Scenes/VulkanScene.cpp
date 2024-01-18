#include "VulkanScene.h"
VulkanScene::VulkanScene()
{
	sceneCamera = Camera::GetCamera();
	logicalDevice = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	CreateCommandPool();
}

VulkanScene::~VulkanScene()
{
	ModelLoader::destroy();
	TextureLoader::destroy();
	BasicShapes::destroyShapes();
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
}

void VulkanScene::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.queueFamilyIndex = VulkanDevice::GetVulkanDevice()->GetFamilyIndices().graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool");
	
	ModelLoader::setCommandPool(commandPool);
	TextureLoader::setCommandPool(commandPool);
	BasicShapes::loadShapes(commandPool);
}

