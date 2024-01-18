#pragma once
#include "VulkanScene.h"

class PBR : public VulkanScene
{
public:
	PBR(const std::string name, const VulkanSwapChain& swapChain);
	~PBR();

	

	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) override;
	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) override;
	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY) override;


private:
	VulkanGraphicsPipeline skyboxPipeline, scenePipeline, postProcessPipeline;
	VkRenderPass renderPass;
	VkFramebuffer framebuffer;
	VkImageView colorAttachment, depthAttachment;

	//VkBuffer someUniformBuffers[]

	Mesh monkey, skyboxCube;

	struct
	{
		glm::mat4 mvp;
	} skyboxUBO;

	Texture* skyboxTexture;
	VkFormat swapChainImageFormat;

	virtual void RecreateScene(const VulkanSwapChain& swapChain) override;
	virtual void DestroyScene(bool isRecreation) override;
	virtual void RecordScene() override;
	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial) override;

	void CreateRenderPass();

	void CreateSkyboxPipeline();
	void CreateScenePipeline();
	void CreatePostProcessPipeline();

	void CreateFramebuffers();
	void CreateCommandBuffers();
	void CreateSyncObjects();

};