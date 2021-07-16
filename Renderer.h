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

#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <algorithm>

#include "VulkanDevice.h"
#include "HelloWorldTriangle.h"

class Renderer
{

public:

	// Constructor initializes SDL and Vulkan, and creates a Vulkan Surface for rendering
	Renderer();
	~Renderer();


	// helper functions to divide initialization work

	// RunApp performs loop
	void RunApp();

	// Free up any allocated memory
	void CleanUp();

	unsigned int windowWidth = 800, windowHeight = 600;

private:

	// Devices
	VulkanDevice* device;

	// Rendering
	SDL_Window* appWindow;
	VkSurfaceKHR renderSurface;

	bool isAppRunning = true;
	
	VulkanSwapChain vulkanSwapChain;
	

	// extensions
	unsigned extensionCount;
	std::vector<const char*> extensionsList;

	// Vulkan Info
	VkInstance instance;
	VkResult result;
	std::vector<const char*> validationLayersList = { "VK_LAYER_KHRONOS_validation", };

	// Scenes
	std::vector<VulkanScene*> scenesList;
	unsigned int sceneIndex = 0;
	bool windowMinimized = false;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	bool checkValidationLayerSupport();

protected:
	void CreateAppWindow();
	void CreateVKInstance();
	void CreateVKSurface();
	void CreateSwapChain();
	void CreateImages();
	void RecreateSwapChain();
	
};



#endif // !RENDERER_H