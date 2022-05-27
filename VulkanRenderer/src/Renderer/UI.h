#pragma once

#include "ImGui/imgui.h"

// backends
#include "vendor/imgui_impl_sdl.h"
#include "vendor/imgui_impl_vulkan.h"

#include "HelperStructs.h"

class UI
{
public:
	UI(const VkCommandPool& commandPool, const VulkanSwapChain& swapChain, const VulkanGraphicsPipeline& graphicsPipeline);
	~UI();

	void NewUIFrame();
	void EndFrame();
	void RenderFrame(VkCommandBuffer commandBuffer, uint32_t index);

private:
	VulkanGraphicsPipeline graphicsPipeline;
	int vertexCount = 0, indexCount = 0;

	Texture fontTexture;
	
	VkCommandPool commandPool;
	VkDevice device;

	struct
	{
		glm::vec2 scale;
		glm::vec2 translate;
	}pushConstBlock;

	void CreateRenderPass(VkFormat swapChainFormat);
	void CreateDescriptors(int numImages);
	void CreateFontTexture(VkQueue renderQueue);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateGraphicsPipeline();

	void UpdateBuffers();
};