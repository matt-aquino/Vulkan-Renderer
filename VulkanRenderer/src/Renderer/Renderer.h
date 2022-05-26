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

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "SDL2/SDL_vulkan.h"
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"
#include <chrono>
#include <algorithm>

//#include "VulkanDevice.h"
#include "Scenes/VulkanScene.h"

class Renderer
{

public:

	// Constructor initializes SDL and Vulkan, and creates a Vulkan Surface for rendering
	Renderer();
	~Renderer();

	static SDL_Window* GetWindow() { return appWindow; }
	static VkInstance GetVKInstance() { return instance; }

	// helper functions to divide initialization work

	// RunApp performs loop
	void RunApp();

	// Free up any allocated memory
	void CleanUp();

	unsigned int windowWidth = 1280, windowHeight = 720;

private:

	// Rendering
	static SDL_Window* appWindow;
	VkSurfaceKHR renderSurface;
	float dt = 0.0f;
	int lastX = 0, lastY = 0;
	std::chrono::steady_clock::time_point startTime, currentTime, lastTime;
	bool isAppRunning = true;
	
	VulkanSwapChain vulkanSwapChain;
	

	// extensions
	unsigned extensionCount;
	std::vector<const char*> extensionsList = { "VK_EXT_extended_dynamic_state" };

	// Vulkan Info
	static VkInstance instance;
	VkResult result;

#define VKDUMP 0 // "VK_LAYER_LUNARG_api_dump" will print the result of every vk... function, to get more thorough results

#if VKDUMP == 1
	std::vector<const char*> validationLayersList = { "VK_LAYER_LUNARG_api_dump", "VK_LAYER_KHRONOS_validation", };
#else
	std::vector<const char*> validationLayersList = { "VK_LAYER_KHRONOS_validation", };
#endif

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