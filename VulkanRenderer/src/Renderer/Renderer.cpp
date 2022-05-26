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

#include "Renderer.h"
#include "Camera.h"

#include "Scenes/Materials.h"
SDL_Window* Renderer::appWindow = nullptr;
VkInstance Renderer::instance = VK_NULL_HANDLE;

Renderer::Renderer()
{
    CreateAppWindow();
    CreateVKInstance();
    CreateVKSurface();
    VulkanDevice::GetVulkanDevice()->CreateVulkanDevice(instance, renderSurface);
    
    CreateSwapChain();
    CreateImages();

    MaterialScene* scene = new MaterialScene("Materials Example", vulkanSwapChain);
    scenesList.push_back(scene);
}


Renderer::~Renderer()
{
    CleanUp();
}

void Renderer::CreateAppWindow()
{
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error("Could not initialize SDL.");

    appWindow = SDL_CreateWindow("Vulkan Renderer", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (appWindow == NULL)
        throw std::runtime_error("Could not create SDL window.");

    SDL_SetWindowResizable(appWindow, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    if (!SDL_Vulkan_GetInstanceExtensions(appWindow, &extensionCount, NULL))
        throw std::runtime_error("Could not get the number of required instance extensions from SDL.");

    extensionsList.resize(extensionCount);

    if (!SDL_Vulkan_GetInstanceExtensions(appWindow, &extensionCount, extensionsList.data()))
        throw std::runtime_error("Could not get the names of required instance extensions from SDL.");
    
}

void Renderer::CreateVKInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available");

    // VkApplicationInfo allows the programmer to specifiy some basic information about the
  // program, which can be useful for layers and tools to provide more debug information.

    VkApplicationInfo appInfo = {};
    VkInstanceCreateInfo instanceInfo = {};

    if (enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersList.size());
        instanceInfo.ppEnabledLayerNames = validationLayersList.data();
    }
    else
        instanceInfo.enabledLayerCount = 0;

    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    //appInfo.pNext = NULL;
    appInfo.pApplicationName = "Vulkan Renderer";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "LunarG SDK";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);

    // VkInstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsList.size());
    instanceInfo.ppEnabledExtensionNames = extensionsList.data();

    // Create the Vulkan instance.
    result = vkCreateInstance(&instanceInfo, NULL, &instance);

    if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
        throw std::runtime_error("Unable to find a compatible Vulkan Driver.");

    else if (result != VK_SUCCESS)
        throw std::runtime_error("Could not create a Vulkan instance (for unknown reasons).");
}

void Renderer::CreateVKSurface()
{
    // Create a Vulkan surface for rendering
    if (!SDL_Vulkan_CreateSurface(appWindow, instance, &renderSurface)) 
        throw std::runtime_error("Could not create a Vulkan surface.");
}

void Renderer::RunApp()
{
    SDL_SetWindowTitle(appWindow, scenesList[sceneIndex]->sceneName.c_str());

    VulkanReturnValues returnValues;
    Camera* const camera = Camera::GetCamera();
    bool isCameraMoving = false;

    // Poll for user input.
    while (isAppRunning) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
                // user hit X window
                case SDL_QUIT:
                    isAppRunning = false;
                    break;

                // scene change
                case SDL_KEYDOWN:
                    switch (event.key.keysym.scancode)
                    {
                        case SDL_SCANCODE_ESCAPE:
                            isAppRunning = false;
                            break;

                        case SDL_SCANCODE_LEFT:

                            if (sceneIndex == 0)
                                sceneIndex = static_cast<uint32_t>(scenesList.size()) - 1;

                            else
                                sceneIndex--;

                            SDL_SetWindowTitle(appWindow, scenesList[sceneIndex]->sceneName.c_str()); // change window title
                            break;

                        case SDL_SCANCODE_RIGHT:
                            sceneIndex = (sceneIndex + 1) % scenesList.size();
                            SDL_SetWindowTitle(appWindow, scenesList[sceneIndex]->sceneName.c_str()); // change window title
                            break;

                    }
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
                    {
                        windowMinimized = true;
                    }

                    if (event.window.event == SDL_WINDOWEVENT_RESTORED)
                    {
                        RecreateSwapChain();
                        windowMinimized = false;
                    }
                    break;


                default:
                    // Do nothing.
                    break;
            }
        }

        currentTime = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        // handle user input before next frame
        int currentMouseX, currentMouseY;
        uint32_t buttons;

        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        buttons = SDL_GetRelativeMouseState(&currentMouseX, &currentMouseY);
        
        // don't run the scene while the scene is minimized
        if (!windowMinimized)
        {
            scenesList[sceneIndex]->HandleKeyboardInput(keystates, dt);
            scenesList[sceneIndex]->HandleMouseInput(buttons, currentMouseX, currentMouseY);

            returnValues = scenesList[sceneIndex]->PresentScene(vulkanSwapChain);
            
            if (returnValues == VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE)
                RecreateSwapChain();
        }
    }

    // wait for operations to finish before exiting
    vkDeviceWaitIdle(VulkanDevice::GetVulkanDevice()->logicalDevice);
}

void Renderer::CleanUp()
{
    VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

    // Clear all scenes
    for (VulkanScene* scene : scenesList)
    {
        delete scene;
    }

    scenesList.clear();

    // destroy swap chain
    // order is very particular...
    for (VkImageView imageView : vulkanSwapChain.swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, vulkanSwapChain.swapChain, nullptr);

    VulkanDevice::GetVulkanDevice()->DeleteLogicalDevice();

    vkDestroySurfaceKHR(instance, renderSurface, NULL);
    vkDestroyInstance(instance, NULL);

    SDL_DestroyWindow(appWindow);
    SDL_Quit();
}


void Renderer::CreateSwapChain()
{
    VkPhysicalDevice physical = VulkanDevice::GetVulkanDevice()->GetPhysicalDevice();
    VkDevice logical = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

    // Query for swap chain capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, renderSurface, &vulkanSwapChain.surfaceCapabilities);
    uint32_t formatCount, presentModesCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, renderSurface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, renderSurface, &presentModesCount, nullptr);

    if (formatCount != 0)
    {
        vulkanSwapChain.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, renderSurface, &formatCount, vulkanSwapChain.surfaceFormats.data());
    }

    else
        throw std::runtime_error("Failed to find surface formats");

    if (presentModesCount != 0)
    {
        vulkanSwapChain.surfacePresentModes.resize(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical, renderSurface, &presentModesCount, vulkanSwapChain.surfacePresentModes.data());
    }

    else
        throw std::runtime_error("Failed to find surface present modes");

    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;

    // choose surface formats
    for (VkSurfaceFormatKHR format : vulkanSwapChain.surfaceFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            surfaceFormat = format;
    }

    // choose presentation modes
    for (VkPresentModeKHR mode : vulkanSwapChain.surfacePresentModes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            presentMode = mode;
    }

    // choose swap extent
    
    if (vulkanSwapChain.surfaceCapabilities.currentExtent.width != UINT32_MAX)
        vulkanSwapChain.swapChainDimensions = vulkanSwapChain.surfaceCapabilities.currentExtent;

    else
    {
        int width, height;
        SDL_Vulkan_GetDrawableSize(appWindow, &width, &height);

        vulkanSwapChain.swapChainDimensions = { (uint32_t)width, (uint32_t)height };
        vulkanSwapChain.swapChainDimensions.width = std::clamp(vulkanSwapChain.swapChainDimensions.width, vulkanSwapChain.surfaceCapabilities.minImageExtent.width,
            vulkanSwapChain.surfaceCapabilities.maxImageExtent.width);

        vulkanSwapChain.swapChainDimensions.height = std::clamp(vulkanSwapChain.swapChainDimensions.height, vulkanSwapChain.surfaceCapabilities.minImageExtent.height,
            vulkanSwapChain.surfaceCapabilities.maxImageExtent.height);
    }
    
    // my computer allows a minimum of 2 images, so we'll allow 3 for swap chain
    uint32_t imageCount = vulkanSwapChain.surfaceCapabilities.minImageCount + 1;

    // make sure we don't have more images than the surface can handle
    if (vulkanSwapChain.surfaceCapabilities.maxImageCount > 0 && imageCount > vulkanSwapChain.surfaceCapabilities.maxImageCount)
        imageCount = vulkanSwapChain.surfaceCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = renderSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent.width = vulkanSwapChain.swapChainDimensions.width;
    createInfo.imageExtent.height = vulkanSwapChain.swapChainDimensions.height;
    createInfo.imageArrayLayers = 1; // amount of layers each image consists of; useful for stereoscopic 3D apps

    // since we're rendering to these images, they are our color buffers
    // use _TRANSFER_DST_BIT for offscreen rendering (postprocessing, etc)
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

    VulkanDevice::QueueFamilyIndices indices = VulkanDevice::GetVulkanDevice()->GetFamilyIndices();

    // check if using 2 separate queues for rendering and presentation
    // use a different sharing mode accordingly
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily.value() != indices.presentFamily.value())
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = (const uint32_t*)nullptr;
    }

    createInfo.preTransform = vulkanSwapChain.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    result = vkCreateSwapchainKHR(logical, &createInfo, nullptr, &vulkanSwapChain.swapChain); // this is breaking...
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    vkGetSwapchainImagesKHR(logical, vulkanSwapChain.swapChain, &imageCount, nullptr);
    vulkanSwapChain.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(logical, vulkanSwapChain.swapChain, &imageCount, vulkanSwapChain.swapChainImages.data());

    vulkanSwapChain.swapChainImageFormat = surfaceFormat.format;
    vulkanSwapChain.swapChainDimensions = vulkanSwapChain.surfaceCapabilities.currentExtent;

}


void Renderer::CreateImages()
{
    size_t imageListSize = vulkanSwapChain.swapChainImages.size();
    vulkanSwapChain.swapChainImageViews.resize(imageListSize);
    for (size_t i = 0; i < imageListSize; i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vulkanSwapChain.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // images can be either 1D, 2D, 3D, and cubemaps
        createInfo.format = vulkanSwapChain.swapChainImageFormat;

        // color channels
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // subresourceRange describe the image's purpose and which part should be accessed
        // we will be using these as color targets with no mipmapping or layers
        // layers are useful for stereoscopic 3D apps (VR and such)
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(VulkanDevice::GetVulkanDevice()->logicalDevice, &createInfo, nullptr, &vulkanSwapChain.swapChainImageViews[i]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create image views!");
    }
}


void Renderer::RecreateSwapChain()
{
    // wait for all operations to finish
    vkDeviceWaitIdle(VulkanDevice::GetVulkanDevice()->logicalDevice);

    for (size_t i = 0; i < vulkanSwapChain.swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(VulkanDevice::GetVulkanDevice()->logicalDevice, vulkanSwapChain.swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(VulkanDevice::GetVulkanDevice()->logicalDevice, vulkanSwapChain.swapChain, nullptr);
    CreateSwapChain();
    CreateImages();

    // all framebuffers are rendered useless now, so recreate them with the new swap chain
    for (int i = 0; i < scenesList.size(); i++)
        scenesList[i]->RecreateScene(vulkanSwapChain);
}

bool Renderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayersList) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

