/*
   Copyright 2021 Matthew Aquino

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef HELLO_WORLD_TRIANGLE_H
#define HELLO_WORLD_TRIANGLE_H

#include "VulkanScene.h"
#include <chrono>

class HelloWorldTriangle : public VulkanScene
{

public:
	HelloWorldTriangle(std::string name, const VulkanSwapChain& swapChain);
	~HelloWorldTriangle() { DestroyScene(false); }

	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) override;

	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) override;
	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY) override;

private:
	
	virtual void DestroyScene(bool isRecreation) override;
	virtual void RecordScene() override;
	virtual void RecreateScene(const VulkanSwapChain& swapChain) override;
	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial = false) override;

	// ** Create all aspects of the graphics pipeline **
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);

	// ** Define the render pass, along with any subpasses, dependencies, and color attachments **
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	 
	// ** Create the framebuffers used for rendering **
	void CreateFramebuffers(const VulkanSwapChain& swapChain);


	void CreateCommandBuffers();

	// ** Create the synchronization object - semaphores, fences **
	void CreateSyncObjects(const VulkanSwapChain& swapChain);

	// ** Create vertex buffers via staging buffers **
	void CreateVertexBuffer();

	// ** Create uniform variables for our shaders **
	void CreateUniforms(const VulkanSwapChain& swapChain);

	// ** Update uniform variables for shaders **
	void UpdateUniforms(uint32_t currentImage);


	VulkanGraphicsPipeline graphicsPipeline;
	VkRenderPass renderPass;

	std::vector<VkFramebuffer> framebuffers;

	VkSampler imageSampler;
	size_t currentFrame = 0;

	const glm::vec3 cameraPosition = { 0.0f, 0.0f, -5.0f };
	
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
	} ubo;

	struct PushConstants
	{
		glm::mat4 projectionMatrix;
	} pushConstants;

	const std::vector<Vertex> vertices = {
		{{0.5f, -0.5f, 0.0f,1.0f}, {0.0f, 0.0f, 1.0f,1.0f}},
		{{0.0f, 0.5f, 0.0f,1.0f}, {0.0f, 1.0f, 0.0f,1.0f}},
		{{-0.5f, -0.5f, 0.0f,1.0f}, {1.0f, 0.0f, 0.0f,1.0f}}
	};

	VulkanBuffer vertexBuffer = {};
};


#endif // HELLO_WORLD_TRIANGLE_H