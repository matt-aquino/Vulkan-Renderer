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

#ifndef MODELED_OBJECT_H
#define MODELED_OBJECT_H

#include "VulkanScene.h"
#include "Model.h"
#include <chrono>

class ModeledObject : public VulkanScene
{
public:
	ModeledObject(std::string name, const VulkanSwapChain& swapChain);
	ModeledObject();
	~ModeledObject();

	void CreateScene() override;

	void RecreateScene(const VulkanSwapChain& swapChain) override;

	VulkanReturnValues RunScene(const VulkanSwapChain& swapChain) override;

	void DestroyScene(bool isRecreation) override;



private:
	Model *object;

	// TO DO: split up functions to separately create UBO, samplers, etc
	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateFramebuffer(const VulkanSwapChain& swapChain);
	void CreateSyncObjects(const VulkanSwapChain& swapChain);
	void CreateUniforms(const VulkanSwapChain& swapChain);
	void UpdateUniforms(uint32_t currentFrame);
	void CreateBuffers();
	void CreateDescriptorSets(const VulkanSwapChain& swapChain);

	void CreateCommandBuffers();

	VulkanGraphicsPipeline graphicsPipeline;

	struct UniformBufferObject
	{
		alignas(16)glm::mat4 model;
		alignas(16)glm::mat4 view;
		alignas(16)glm::mat4 projection;
		alignas(16)glm::vec3 cameraPosition;
	} ubo;

	
	size_t currentFrame = 0;
};


#endif //!MODELED_OBJECT_H