#pragma once

#include "VulkanScene.h"
#include "Renderer/UI.h"

class MaterialScene : public VulkanScene // TO DO: flesh out this class, and try different presets
{
public:
	MaterialScene(std::string name, const VulkanSwapChain& swapChain);
	~MaterialScene();

	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) override;
	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) override;
	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY) override;

private:

	// Inherited via VulkanScene
	virtual void RecreateScene(const VulkanSwapChain& swapChain) override;
	virtual void RecordScene() override;
	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial = false) override;
	virtual void DestroyScene(bool isRecreation) override;

	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateDescriptorSets(const VulkanSwapChain& swapChain);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);
	void CreateFramebufferResources(const VulkanSwapChain& swapChain);

	void CreateSyncObjects(const VulkanSwapChain& swapChain);

	void CreateObjects();
	void CreateUniforms(const VulkanSwapChain& swapChain);
	void UpdateUniforms(uint32_t index);

	void CreateCommandBuffers();
	void RecordCommandBuffers(uint32_t index);

	void DrawUI(uint32_t index);

	// scene data
	VulkanGraphicsPipeline graphicsPipeline;
	VkRenderPass renderPass;

	SpotLight light;
	std::vector<Mesh> objects;
	uint32_t currentFrame = 0;
	bool animate = true;
	bool isCameraMoving = false;

	UI* ui = nullptr;

	struct // scene uniform buffer data
	{
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec3 lightPos;
		glm::vec3 viewPos;
	} uboScene;
	
	glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// MSAA targets
	Texture msaaTex = {};

};