#ifndef STENCILBUFFER_H
#define STENCILBUFFER_H

#include "VulkanScene.h"
#include "HelperStructs.h"

class StencilBuffer : public VulkanScene
{
public:
	StencilBuffer(std::string name, const VulkanSwapChain& swapChain);
	~StencilBuffer();

	void RecordScene() override;
	void DestroyScene(bool isRecreation) override;
	void RecreateScene(const VulkanSwapChain& swapChain) override;
	VulkanReturnValues DrawScene(const VulkanSwapChain& swapChain) override;

private:

	void CreateCommandBuffers();
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateGraphicsPipelines(const VulkanSwapChain& swapChain);
    void CreateDescriptorSets(const VulkanSwapChain& swapChain);
	void CreateSyncObjects(const VulkanSwapChain& swapChain);
	void CreateUniforms(const VulkanSwapChain& swapChain);
	void UpdateUniforms(uint32_t currentFrame);

	VulkanGraphicsPipeline colorPipeline, stencilPipeline;

	Model *model;

	struct UBO
	{
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
		float outlineWidth = 0.25f;
	} ubo;

    uint32_t currentFrame = 0;
};



#endif //!STENCILBUFFER_H