#ifndef STENCILBUFFER_H
#define STENCILBUFFER_H

#include "VulkanScene.h"
#include "HelperStructs.h"
#include "Camera.h"

class StencilBuffer : public VulkanScene
{
public:
	StencilBuffer(std::string name, const VulkanSwapChain& swapChain);
	~StencilBuffer();

	void CreateScene() override;
	void DestroyScene(bool isRecreation) override;
	void RecreateScene(const VulkanSwapChain& swapChain) override;
	VulkanReturnValues RunScene(const VulkanSwapChain& swapChain) override;

private:

	void CreateCommandBuffers();
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);
	void CreateSyncObjects(const VulkanSwapChain& swapChain);
	void CreatePushConstants(const VulkanSwapChain& swapChain);
	void UpdatePushConstants();
    void CreateVertexData();

	VulkanGraphicsPipeline graphicsPipeline;
	struct PushConstants
	{
        glm::mat4 mvp;
	} pushConstants;


    glm::mat4 model, projection;

    std::vector<float> cubeVertices = {
        -0.5f, -0.5f, -0.5f,    // 0 
         0.5f, -0.5f, -0.5f,    // 1
         0.5f,  0.5f, -0.5f,    // 2
        -0.5f,  0.5f, -0.5f,    // 3
        -0.5f, -0.5f,  0.5f,    // 4
         0.5f, -0.5f,  0.5f,    // 5
         0.5f,  0.5f,  0.5f,    // 6
        -0.5f,  0.5f,  0.5f,    // 7
    };

    std::vector<int> indices = {
        0, 1, 2,
        2, 3, 0,
        4, 5, 6,
        6, 7, 4,
        7, 3, 0,
        0, 4, 7,
        6, 2, 1,
        1, 5, 6,
        0, 1, 5,
        5, 4, 0,
        3, 2, 6,
        6, 7, 3
    };

    std::vector<VkCommandBuffer> secondaryCmdBuffers;

    uint32_t currentFrame = 0;
};



#endif //!STENCILBUFFER_H