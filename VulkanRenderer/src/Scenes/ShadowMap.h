#pragma once
#include "VulkanScene.h"
#include "Renderer/UI.h"

// Shadow Mapping requires us to render the scene offscreen from a light's perspective
// and determine which areas of our scene are occluded (light is blocked)
class ShadowMap : public VulkanScene
{
public:
	ShadowMap();
	ShadowMap(std::string name, const VulkanSwapChain& swapChain);
	~ShadowMap();

	virtual void RecordScene() override;
	virtual void RecreateScene(const VulkanSwapChain& swapChain) override;
	virtual void DestroyScene(bool isRecreation) override;

	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial = false) override;
	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) override;

	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) override;
	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY) override;

private:

	void CreatePipelines(const VulkanSwapChain& swapChain);

	void CreateSceneRenderPass(const VulkanSwapChain& swapChain);
	void CreateSceneDescriptorSets(const VulkanSwapChain& swapChain);
	void CreateSceneFramebuffers(const VulkanSwapChain& swapChain);

	void CreateDebugResources(const VulkanSwapChain& swapChain);

	void CreateShadowRenderPass(const VulkanSwapChain& swapChain);
	void CreateShadowResources();
	void CreateShadowDescriptorSets(const VulkanSwapChain& swapChain);
	void CreateShadowFramebuffers(const VulkanSwapChain& swapChain);


	void CreateSyncObjects(const VulkanSwapChain& swapChain);

	void CreateUniforms(const VulkanSwapChain& swapChain);
	void CreateSceneObjects();
	void UpdateUniforms(uint32_t index);

	void CreateCommandBuffers();

	void RecordCommandBuffers(uint32_t index);

	// scene data
	VulkanGraphicsPipeline graphicsPipeline, debugPipeline;
	Texture msaaTex, debugTex;

	bool isCameraMoving = false;

	SpotLight light = {};

	struct // scene uniform buffer data
	{
		glm::mat4 proj;
		glm::mat4 view;
		glm::mat4 lightVP;
		glm::vec4 lightPos;
	} uboScene;

	Mesh cube, ground, monkey, sphere;
	size_t currentFrame = 0;

	// shadow mapping data
	float depthBiasConstant = 1.25f; // constant depth bias factor, always applied
	float depthBiasSlope = 1.75f;    // slope depth bias factor, applied depending on polygon's slope
	uint32_t shadowMapDim = 2048;	 // higher dimensions means better shadows, but more expensive

	struct // shadow pass uniform data
	{
		glm::mat4 viewProj;
	} uboShadow;

	VulkanGraphicsPipeline shadowPipeline;
	VkSampler shadowSampler;
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	// UI
	UI* ui = nullptr;
	void DrawUI(uint32_t frameIndex);
};