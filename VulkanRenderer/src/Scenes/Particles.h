#ifndef PARTICLES_H
#define PARTICLES_H

#include "VulkanScene.h"
#include "VulkanDevice.h"
#include <time.h>

class Particles : public VulkanScene
{
public:
	Particles(std::string name, const VulkanSwapChain & swapChain);
	~Particles();

	void RecordScene() override;
	void RecreateScene(const VulkanSwapChain& swapChain) override;
	VulkanReturnValues DrawScene(const VulkanSwapChain& swapChain) override;
	void DestroyScene(bool isRecreation) override;

	void HandleKeyboardInput(const uint8_t* keystates, float dt) override;
	void HandleMouseInput(const int x, const int y) override;

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
	void CreateUniforms(const VulkanSwapChain& swapChain);
	void UpdateUniforms(uint32_t currentFrame);

	void ReadBackParticleData();

	uint32_t currentFrame = 0;

	VulkanGraphicsPipeline graphicsPipeline;
	VulkanComputePipeline computePipeline;

	struct UBO
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	} ubo;

	struct ComputePush
	{
		int xBounds;
		int yBounds;
		int zBounds;
	} pushConstants;

	glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, -10.0f);

	std::vector<Particle> particles;
	VkBufferMemoryBarrier computeFinishedBarrier, vertexFinishedBarrier;
};


#endif //!PARTICLES_H