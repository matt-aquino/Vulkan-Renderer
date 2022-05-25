#pragma once
#include "VulkanScene.h"
#include <array>

class Mandelbrot : public VulkanScene
{
public:
	Mandelbrot();
	Mandelbrot(std::string name, const VulkanSwapChain& swapChain);
	~Mandelbrot();

	// ** Perform initial setup of a scene
	virtual void RecordScene() override;

	// ** Recreate the scene when swap chain goes out of date **
	virtual void RecreateScene(const VulkanSwapChain& swapChain) override;

	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial = false) override;

	// ** perform main loop of scene **
	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) override;

	// ** Clean up resources ** 
	virtual void DestroyScene(bool isRecreation) override;

	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) override;

	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y) override;

private:
	// ** Create all aspects of the graphics and compute pipelines **
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);

	// ** Define the render pass, along with any subpasses, dependencies, and color attachments **
	void CreateRenderPass(const VulkanSwapChain& swapChain);

	// update our uniform buffer
	void UpdateUniforms(uint32_t currentFrame);

	// create descriptor sets for graphics pipeline
	void CreateDescriptorSets(const VulkanSwapChain& swapChain);

	// ** Create the framebuffers used for rendering **
	void CreateFramebuffers(const VulkanSwapChain& swapChain);

	// ** create command buffers used for recording draw commands
	void CreateCommandBuffers();

	// ** Create the synchronization object - semaphores, fences **
	void CreateSyncObjects(const VulkanSwapChain& swapChain);

	VulkanGraphicsPipeline graphicsPipeline;

	struct PushConstants
	{
		float width;
		float height;
	} pushConstants;

	struct UBO
	{
		float xOffset;
		float yOffset;
		float zoom;
		int numIterations;
		float threshold;
	} ubo;

	size_t currentFrame = 0;
};