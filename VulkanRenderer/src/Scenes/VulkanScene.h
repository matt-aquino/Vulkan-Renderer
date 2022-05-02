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
#include "SDL_scancode.h"

#define SHADERPATH "shaders/"

class VulkanScene
{
public:
	VulkanScene();
	~VulkanScene();

	// ** Perform initial setup of a scene
	virtual void RecordScene() = 0;

	// ** Recreate the scene when swap chain goes out of date **
	virtual void RecreateScene(const VulkanSwapChain& swapChain) = 0;

	// ** perform main loop of scene **
	virtual VulkanReturnValues DrawScene(const VulkanSwapChain& swapChain) = 0;
	
	// ** Clean up resources ** 
	virtual void DestroyScene(bool isRecreation) = 0;

	// handle user inputs
	virtual void HandleKeyboardInput(const uint8_t* keystates, float dt) = 0;
	virtual void HandleMouseInput(const int x, const int y) = 0;

	std::string sceneName;

protected:

	// ** Allocate memory to a command pool for command buffers **
	void CreateCommandPool();

	// Command Buffers
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffersList;

	// Synchronzation Objects
	std::vector<VkSemaphore> renderCompleteSemaphores, presentCompleteSemaphores;
	std::vector<VkFence> inFlightFences, imagesInFlight;
	const int MAX_FRAMES_IN_FLIGHT = 3; // we need 1 fence per frame. since we're triple buffering, that means 3 fences
};

#endif // !VULKANSCENE_H