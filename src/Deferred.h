#ifndef DEFERRED_H
#define DEFERRED_H


#include "VulkanScene.h"
#include "HelperStructs.h"
#include "Light.h"

class Deferred : public VulkanScene
{
public:
	Deferred(std::string name, const VulkanSwapChain& swapChain);
	~Deferred();

	void RecordScene() override;
	void DestroyScene(bool isRecreation) override;
	void RecreateScene(const VulkanSwapChain& swapChain) override;
	VulkanReturnValues DrawScene(const VulkanSwapChain& swapChain) override;

private:

	void CreateCommandBuffers();
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateGBufferImages(const VulkanSwapChain& swapChain);
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);
	void CreateDescriptorSets(const VulkanSwapChain& swapChain);
	void CreateSyncObjects(const VulkanSwapChain& swapChain);
	void CreateUniforms(const VulkanSwapChain& swapChain);
	void UpdateUniforms(uint32_t currentFrame);

	VulkanGraphicsPipeline graphicsPipeline;
	std::vector<Light> lights;
	uint32_t currentFrame = 0;

	std::vector<VkImage> gbufferImages;
	std::vector<VkImageView> gbufferImageViews;
	std::vector<VkDeviceMemory> gbufferMemory;

	struct UBO
	{
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);
	} ubo;
};


#endif //!DEFERRED_H