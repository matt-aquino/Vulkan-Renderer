#pragma once
#include "VulkanScene.h"
#include <random>
#include "Renderer/UI.h"

/* TO DO
* since render pass was removed from graphics pipeline helper struct,
* we need to refactor code so both pipelines share the same render pass
* need to use multiple subpasses?
*/
class DeferredRendering : public VulkanScene
{
public:
	DeferredRendering(const std::string sceneName, const VulkanSwapChain& swapChain);
	~DeferredRendering();
	
	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) override;
	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) override;
	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY) override;

private:
	// offscreen pipeline renders the scene to G-buffer textures
	// composition pipeline renders to a full screen quad to display the scene
	VulkanGraphicsPipeline offscreenPipeline, compositionPipeline;
	VkRenderPass renderPass;

	Camera* sceneCamera = nullptr;
	glm::mat4 proj;
	Mesh spheres[100], plane;

	// g-buffer textures
	Texture colorTexture, normalTexture, positionTexture;
	VkSampler textureSampler;

	struct
	{
		glm::mat4 viewProj;
		glm::mat4 modelMats[100];
		glm::mat4 normalMats[100];
	} deferredUBO;

	struct SceneLight
	{
		glm::vec4 position;
		glm::vec4 color;
		float radius;
		float intensity;
	};

	struct
	{
		SceneLight lights[100];
	} compositionUBO;

	UI* ui = nullptr;
	bool isCameraMoving = false;
	uint32_t currentFrame = 0;

	virtual void RecordScene() override;
	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial) override;
	virtual void RecreateScene(const VulkanSwapChain& swapChain) override;
	virtual void DestroyScene(bool isRecreation) override;

	// deferred pipeline creation
	void CreateOffscreenPipelineResources(const VulkanSwapChain& swapChain);
	void CreateOffscreenRenderPass(const VulkanSwapChain& swapChain);
	void CreateOffscreenPipeline(const VulkanSwapChain& swapChain);
	void CreateOffscreenFramebuffers(const VulkanSwapChain& swapChain);

	// composition pipeline creation
	void CreateCompositionPipelineResources(const VulkanSwapChain& swapChain);
	void CreateCompositionRenderPass(const VulkanSwapChain& swapChain);
	void CreateCompositionPipeline(const VulkanSwapChain& swapChain);
	void CreateCompositionFramebuffers(const VulkanSwapChain& swapChain);

	void CreateSceneObjects(const VulkanSwapChain& swapChain);
	void CreateSyncObjects();
	void CreateCommandBuffers();

	void Update(uint32_t index);
	void RecordCommandBuffer(uint32_t index);
	void DrawUI(uint32_t index);
};