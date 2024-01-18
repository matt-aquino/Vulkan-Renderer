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

#ifndef VULKANSCENE_H
#define VULKANSCENE_H

#include <vulkan/vulkan.h>
#include "VulkanDevice.h"
#include "Renderer/Camera.h"
#include "Renderer/Loaders.h"
#include "Renderer/BasicShapes.h"
#include "Renderer/Light.h"
#include "SDL_scancode.h"
#include "SDL_mouse.h"
#include "Renderer/Timer.h"
#include "Renderer/UI.h"

#define SHADERPATH "shaders/"

class VulkanScene
{
public:
	VulkanScene();
	virtual ~VulkanScene();

	// ** perform main loop of scene **
	virtual VulkanReturnValues PresentScene(const VulkanSwapChain& swapChain) = 0;

	// ** Recreate the scene when swap chain goes out of date **
	virtual void RecreateScene(const VulkanSwapChain& swapChain) = 0;

	
	// handle user inputs
	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) = 0;
	virtual void HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY) = 0;

	std::string sceneName;

protected:

	// ** Allocate memory to a command pool for command buffers **
	void CreateCommandPool();

	// ** Clean up resources ** 
	virtual void DestroyScene(bool isRecreation) = 0;
	
	// ** Perform initial setup of a scene
	virtual void RecordScene() = 0;

	virtual void DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial = false) = 0;

	// Command Buffers
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffersList;
	VkDevice logicalDevice;

	// Synchronzation Objects
	std::vector<VkSemaphore> renderCompleteSemaphores, presentCompleteSemaphores;
	std::vector<VkFence> inFlightFences, imagesInFlight;
	const int MAX_FRAMES_IN_FLIGHT = 3; // we need 1 fence per frame. since we're triple buffering, that means 3 fences
	
	Camera* sceneCamera;
	bool isCameraMoving = true;
};

#endif // !VULKANSCENE_H