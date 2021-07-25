#ifndef PARTICLES_H
#define PARTICLES_H

#include "VulkanScene.h"
#include "VulkanDevice.h"
#include "HelperStructs.h"

class Particles : public VulkanScene
{
public:
	Particles(std::string name, const VulkanSwapChain & swapChain);
	~Particles();

	void CreateScene() override;
	void RecreateScene(const VulkanSwapChain& swapChain) override;
	VulkanReturnValues RunScene(const VulkanSwapChain& swapChain) override;
	void DestroyScene(bool isRecreation) override;


private:
	void CreateCommandBuffers();
	void CreateParticles(bool isRecreation);
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);
	void CreateComputePipeline();
	void CreateSyncObjects(const VulkanSwapChain& swapChain);
	void CreateGraphicsDescriptorSets(const VulkanSwapChain& swapChain);
	void CreateComputeDescriptorSets(const VulkanSwapChain& swapChain);
	void CreatePushConstants(const VulkanSwapChain& swapChain);

	uint32_t currentFrame = 0;

	VulkanGraphicsPipeline graphicsPipeline;
	VulkanComputePipeline computePipeline;

	struct PushConstants
	{
		glm::mat4 mvp;
	} pushConstants;

	std::vector<Particle> particles;
	VkBufferMemoryBarrier computeFinishedBarrier;
};


#endif //!PARTICLES_H