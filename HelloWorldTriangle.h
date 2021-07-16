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
	void CreateVertexBuffer();
	

	VulkanGraphicsPipeline graphicsPipeline;
	size_t currentFrame = 0;
	const int MAX_FRAMES_IN_FLIGHT = 2;

	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription bindDesc = {};
			bindDesc.binding = 0;
			bindDesc.stride = sizeof(Vertex);
			bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindDesc;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attrDesc{};
			attrDesc[0].binding = 0;
			attrDesc[0].location = 0; // match the location within the shader
			attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT; // match format within shader (float, vec2, vec3, vec4,)
			attrDesc[0].offset = offsetof(Vertex, position);

			attrDesc[1].binding = 0;
			attrDesc[1].location = 1;
			attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attrDesc[1].offset = offsetof(Vertex, color);

			return attrDesc;
		}
	};

	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};
};


#endif // HELLO_WORLD_TRIANGLE_H