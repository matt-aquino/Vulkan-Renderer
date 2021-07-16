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

class HelloWorldTriangle : public VulkanScene
{

public:
	HelloWorldTriangle();
	HelloWorldTriangle(std::string name, const VkInstance& instance, const VkSurfaceKHR& appSurface, const VulkanSwapChain& swapChain);

	void CreateScene() override;
	void RecreateScene(const VulkanSwapChain& swapChain) override;
	VulkanReturnValues RunScene(const VulkanSwapChain& swapChain) override;
	void DestroyScene() override;

private:

	void CreateGraphicsPipeline(const VulkanSwapChain& swapChain);
	void CreateRenderPass(const VulkanSwapChain& swapChain);
	void CreateFramebuffers(const VulkanSwapChain& swapChain);
	void CreateShaderModules(const std::vector<char>& code) override;
	void CreateCommandPool() override;
	void CreateSyncObjects(const VulkanSwapChain& swapChain);


	VulkanGraphicsPipeline graphicsPipeline;
	size_t currentFrame = 0;
	const int MAX_FRAMES_IN_FLIGHT = 2;
};


#endif // HELLO_WORLD_TRIANGLE_H