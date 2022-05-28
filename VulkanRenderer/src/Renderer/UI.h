#pragma once

#include "ImGui/imgui.h"

// backends
#include "vendor/imgui_impl_sdl.h"
#include "vendor/imgui_impl_vulkan.h"

#include "HelperStructs.h"
#include "glm/glm.hpp"

class UI
{
public:
	UI(const VkCommandPool& commandPool, const VulkanSwapChain& swapChain, const VulkanGraphicsPipeline& graphicsPipeline);
	~UI();

	void NewUIFrame();
	void EndFrame();
	void RenderFrame(VkCommandBuffer commandBuffer, uint32_t index);

	bool NewWindow(const char* name);
	void EndWindow();

	void NewText(const char* text);
	bool NewCheckBox(const char* label, bool* value);

	void ShowDemoWindow();
	void DisplayFPS();

	bool NewSliderFloat(const char* name, float* value, float minValue, float maxValue);
	bool NewSliderVec2(const char* name, glm::vec2* values, float minValue, float maxValue);
	bool NewSliderVec3(const char* name, glm::vec3* values, float minValue, float maxValue);
	bool NewSliderVec4(const char* name, glm::vec4* values, float minValue, float maxValue);

	bool NewTreeNode(const char* name);
	bool NewTreeNode(const void* ptr, const char* label, ...);
	void EndTreeNode();

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