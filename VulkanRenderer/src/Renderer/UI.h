#pragma once

#include "ImGui/imgui.h"

// backends
#include "vendor/imgui_impl_sdl.h"
#include "vendor/imgui_impl_vulkan.h"

#include "HelperStructs.h"
#include "glm/glm.hpp"
#include <unordered_map>

class UI
{
public:
	UI(const VkCommandPool& commandPool, const VulkanSwapChain& swapChain, const VkRenderPass& renderPass, 
		const VulkanGraphicsPipeline& graphicsPipeline, VkSampleCountFlagBits counts = HelperFunctions::getMaximumSampleCount());
	~UI();

	void NewUIFrame();
	void EndFrame();
	void RenderFrame(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t index);

	bool NewWindow(const char* name);
	void EndWindow();
	
	void AddSpacing(int numSpaces);

	void DrawUIText(const char* text);

	void DrawUITextFloat(const char* name, float& value);
	void DrawUITextVec2(const char* name, glm::vec2& values);
	void DrawUITextVec3(const char* name, glm::vec3& values);
	void DrawUITextVec4(const char* name, glm::vec4& values);

	bool DrawCheckBox(const char* label, bool* value);
	
	
	void DrawImage(const char* name, Texture texture, VkSampler* sampler, glm::vec2 size, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	void DrawImage(const char* name, VkSampler* sampler, VkImageView* imageView, glm::vec2 size, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	void ShowDemoWindow();
	void DisplayFPS();

	bool DrawSliderFloat(const char* name, float* value, float minValue, float maxValue);
	bool DrawSliderVec2(const char* name, glm::vec2* values, float minValue, float maxValue);
	bool DrawSliderVec3(const char* name, glm::vec3* values, float minValue, float maxValue);
	bool DrawSliderVec4(const char* name, glm::vec4* values, float minValue, float maxValue);

	bool NewTreeNode(const char* name);
	bool NewTreeNode(const void* ptr, const char* label, ...);
	void EndTreeNode();

	void SetMouseCapture(bool capture);
	void SetKeyboardCapture(bool capture);
	void SetTextCapture(bool capture);
	void AddMouseButtonEvent(int button, bool pressed);
	void AddMouseWheelEvent(float mouseWheelX, float mouseWheelY);
	void AddKeyEvent(int key, bool pressed);


private:
	VulkanGraphicsPipeline graphicsPipeline;
	VulkanBuffer vertexBuffer = {}, indexBuffer = {};
	VkRenderPass renderPass;
	VkDescriptorPool descriptorPool;
	int vertexCount = 0, indexCount = 0;

	Texture fontTexture;
	VkSampler fontTextureSampler;

	VkCommandPool commandPool;
	VkDevice device;

	std::unordered_map<VkImageView, ImTextureID> addedTextures;

	struct
	{
		glm::vec2 scale;
		glm::vec2 translate;
	}pushConstBlock;


	ImTextureID AddImage(VkSampler& sampler, VkImageView& imageView, VkImageLayout layout);
};