#include "Materials.h"
#include <random>
#include "Renderer/Renderer.h"

MaterialScene::MaterialScene(std::string name, const VulkanSwapChain& swapChain)
{
	sceneName = name;
	srand(unsigned int(NULL));

	CreateObjects();
	CreateUniforms(swapChain);
	CreateSyncObjects(swapChain);

	CreateRenderPass(swapChain);
	CreateFramebuffers(swapChain);
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);

	CreateCommandBuffers();

	ui = new UI(commandPool, swapChain, graphicsPipeline);
}

MaterialScene::~MaterialScene()
{
	DestroyScene(false);
}


void MaterialScene::RecreateScene(const VulkanSwapChain& swapChain)
{
	DestroyScene(true);

	CreateUniforms(swapChain);

	CreateRenderPass(swapChain);
	CreateFramebuffers(swapChain);
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);

	CreateCommandBuffers();

	ui = new UI(commandPool, swapChain, graphicsPipeline);
}

void MaterialScene::RecordScene()
{
}

VulkanReturnValues MaterialScene::PresentScene(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	/* Scene Main Loop
	*  1. Acquire image from swap chain
	*  2. Select appropriate command buffer, execute it
	*  3. Return image to swap chain for presentation
	*/

	uint32_t imageIndex;
	graphicsPipeline.result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;
	}

	else if (graphicsPipeline.result != VK_SUCCESS && graphicsPipeline.result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	// check if a previous frame is using this image
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	UpdateUniforms(imageIndex); // update matrices as needed
	RecordCommandBuffers(imageIndex);

	// mark image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { presentCompleteSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffersList[imageIndex];

	VkSemaphore signalSemaphores[] = { renderCompleteSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	graphicsPipeline.result = vkQueuePresentKHR(VulkanDevice::GetVulkanDevice()->GetQueues().presentQueue, &presentInfo);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || graphicsPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void MaterialScene::DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
	for (int i = 0; i < objects.size(); i++)
	{
		objects[i].draw(commandBuffer, pipelineLayout, useMaterial);
	}
}

void MaterialScene::DestroyScene(bool isRecreation)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	graphicsPipeline.destroyGraphicsPipeline(device);
	msaaTex.destroyTexture();

	uint32_t size = static_cast<uint32_t>(commandBuffersList.size());
	vkFreeCommandBuffers(device, commandPool, size, commandBuffersList.data());

	delete ui;
	// these only NEED to be deleted once cleanup happens
	if (!isRecreation)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		for (int i = 0; i < objects.size(); i++)
		{
			objects[i].destroyMesh();
		}
	}
}

void MaterialScene::HandleKeyboardInput(const uint8_t* keystates, float dt)
{
	static Camera* camera = Camera::GetCamera();
	ImGuiIO& io = ImGui::GetIO();

	if (isCameraMoving)
	{
		io.WantCaptureKeyboard = false;
		io.WantTextInput = false;

		if (keystates[SDL_SCANCODE_A])
			camera->HandleInput(KeyboardInputs::LEFT, dt);

		else if (keystates[SDL_SCANCODE_D])
			camera->HandleInput(KeyboardInputs::RIGHT, dt);

		if (keystates[SDL_SCANCODE_S])
			camera->HandleInput(KeyboardInputs::BACKWARD, dt);

		else if (keystates[SDL_SCANCODE_W])
			camera->HandleInput(KeyboardInputs::FORWARD, dt);

		if (keystates[SDL_SCANCODE_Q])
			camera->HandleInput(KeyboardInputs::DOWN, dt);

		else if (keystates[SDL_SCANCODE_E])
			camera->HandleInput(KeyboardInputs::UP, dt);
	}

	else
	{
		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;
	}

	if (keystates[SDL_SCANCODE_LEFT])
		animate = false;

	else if (keystates[SDL_SCANCODE_RIGHT])
		animate = true;
}

void MaterialScene::HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY)
{
	static Camera* camera = Camera::GetCamera();
	ImGuiIO& io = ImGui::GetIO(); // TO DO: continue working on ImGui input. rework this class into a UI class

	if ((buttons & SDL_BUTTON_RMASK) != 0)
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);
		isCameraMoving = true;
		io.WantCaptureMouse = false;

		static float deltaX = 0.0f;
		static float deltaY = 0.0f;

		// check if current motion is less than/greater than last motion
		float sensivity = 0.1f;

		deltaX = x * sensivity;
		deltaY = y * sensivity;

		camera->RotateCamera(deltaX, deltaY);
	}

	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
		isCameraMoving = false;
		io.WantCaptureMouse = true;
		io.AddMouseButtonEvent(0, (buttons & SDL_BUTTON_LMASK) != 0);
		io.AddMouseWheelEvent(mouseWheelX, mouseWheelY);
	}

	
}

void MaterialScene::CreateRenderPass(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkSampleCountFlagBits counts = HelperFunctions::getMaximumSampleCount();

	// color buffer - multisampled
	VkAttachmentDescription colorAttachment;
	colorAttachment.format = swapChain.swapChainImageFormat;
	colorAttachment.flags = 0;
	colorAttachment.samples = counts;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear existing contents
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store contents in memory for use later
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // existing contents are undefined; we don't care about them
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0; 
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // we will use this image as a color buffer

	// depth buffer - multisampled
	VkAttachmentDescription depthAttachment;
	depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthAttachment.flags = 0;
	depthAttachment.samples = counts;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference;
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// color resolve image - 1 sample
	VkAttachmentDescription colorResolve;
	colorResolve.format = swapChain.swapChainImageFormat;
	colorResolve.flags = 0;
	colorResolve.samples = VK_SAMPLE_COUNT_1_BIT; // image gets resolved to 1 sample
	colorResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // existing contents are undefined; we don't care about them
	colorResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // image will be presented in the swap chain

	VkAttachmentReference colorResolveReference;
	colorResolveReference.attachment = 2;
	colorResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;
	subpass.pResolveAttachments = &colorResolveReference;

	VkSubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment, colorResolve };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = 3;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &graphicsPipeline.renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}

void MaterialScene::CreateDescriptorSets(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	uint32_t descriptorCount = static_cast<uint32_t>(swapChain.swapChainImages.size());

	// create descriptor sets for UBO
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");

	VkDeviceSize bufferSize = sizeof(uboScene);
	graphicsPipeline.uniformBuffers.resize(descriptorCount);

	for (size_t i = 0; i < descriptorCount; i++)
	{
		HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			graphicsPipeline.uniformBuffers[i].buffer, graphicsPipeline.uniformBuffers[i].bufferMemory);

		vkMapMemory(device, graphicsPipeline.uniformBuffers[i].bufferMemory, 0, sizeof(uboScene), 0, &graphicsPipeline.uniformBuffers[i].mappedMemory);
		memcpy(graphicsPipeline.uniformBuffers[i].mappedMemory, &uboScene, sizeof(uboScene));
		vkUnmapMemory(device, graphicsPipeline.uniformBuffers[i].bufferMemory);
	}

	// allocate a descriptor set for each frame
	VkDescriptorPoolSize poolSizes[2] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = descriptorCount;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = descriptorCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 2 * descriptorCount;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");
	graphicsPipeline.isDescriptorPoolEmpty = false;
	
	std::vector<VkDescriptorSetLayout> layout(descriptorCount, graphicsPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = descriptorCount;
	allocInfo.pSetLayouts = layout.data();
	graphicsPipeline.descriptorSets.resize(descriptorCount);

	if (vkAllocateDescriptorSets(device, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

	for (size_t i = 0; i < descriptorCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = graphicsPipeline.uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(uboScene);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = graphicsPipeline.descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}

void MaterialScene::CreateFramebuffers(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkExtent2D dim = swapChain.swapChainDimensions;
	graphicsPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());

	CreateFramebufferResources(swapChain);

	VkImageView attachments[3];

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		attachments[0] = msaaTex.imageView;
		attachments[1] = graphicsPipeline.depthStencilBufferView;
		attachments[2] =  swapChain.swapChainImageViews[i];

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = graphicsPipeline.renderPass;
		framebufferInfo.attachmentCount = 3;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = dim.width;
		framebufferInfo.height = dim.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &graphicsPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}
}

void MaterialScene::CreateFramebufferResources(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;

	// color buffer
	VkSampleCountFlagBits counts = HelperFunctions::getMaximumSampleCount();

	HelperFunctions::createImage(dim.width, dim.height, 1, 1, counts,
		VK_IMAGE_TYPE_2D, swapChain.swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, msaaTex.image, msaaTex.imageMemory);

	HelperFunctions::createImageView(msaaTex.image, msaaTex.imageView, 
		swapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);


	// depth buffer
	HelperFunctions::createImage(dim.width, dim.height, 1, 1, counts,
		VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferMemory);

	HelperFunctions::createImageView(graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferView,
		VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	graphicsPipeline.isDepthBufferEmpty = false;
}

void MaterialScene::CreateGraphicsPipeline(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkSampleCountFlagBits counts = HelperFunctions::getMaximumSampleCount();
	VkExtent2D dim = swapChain.swapChainDimensions;

#pragma region SETUP
	VkVertexInputBindingDescription bindingDescription = ModelVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescription = ModelVertex::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputState = HelperFunctions::initializers::pipelineVertexInputStateCreateInfo(1, bindingDescription, 3, attributeDescription.data());

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = HelperFunctions::initializers::pipelineInputAssemblyStateCreateInfo();
	VkPipelineRasterizationStateCreateInfo rasterizerState = HelperFunctions::initializers::pipelineRasterizationStateCreateInfo();
	VkPipelineMultisampleStateCreateInfo multisampleState = HelperFunctions::initializers::pipelineMultisampleStateCreateInfo(counts);
	multisampleState.sampleShadingEnable = VK_TRUE;
	multisampleState.minSampleShading = 0.2f;


	VkPipelineColorBlendAttachmentState colorBlendingAttachment = {};
	colorBlendingAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendingAttachment.blendEnable = VK_TRUE;
	colorBlendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendingAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendingAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendingAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendState = HelperFunctions::initializers::pipelineColorBlendStateCreateInfo(1, colorBlendingAttachment);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = HelperFunctions::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);


	// viewport
	graphicsPipeline.viewport.x = 0.0f;
	graphicsPipeline.viewport.y = 0.0f;
	graphicsPipeline.viewport.minDepth = 0.0f;
	graphicsPipeline.viewport.maxDepth = 1.0f;
	graphicsPipeline.viewport.width = (float)dim.width;
	graphicsPipeline.viewport.height = (float)dim.height;
	graphicsPipeline.scissors.offset = { 0, 0 };
	graphicsPipeline.scissors.extent = dim;

	VkPipelineViewportStateCreateInfo viewportState = HelperFunctions::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	viewportState.pViewports = &graphicsPipeline.viewport;
	viewportState.pScissors = &graphicsPipeline.scissors;
	

	VkDescriptorSetLayout setLayouts[] = { graphicsPipeline.descriptorSetLayout, objects[0].descriptorSetLayout, objects[0].material->descriptorSetLayout};
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pushConstantRangeCount = 0;
	layoutInfo.pPushConstantRanges = nullptr;
	layoutInfo.setLayoutCount = 3;
	layoutInfo.pSetLayouts = setLayouts;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline layout");

#pragma endregion

#pragma region PIPELINE_CREATION
	auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"MaterialScene/scene_vert.spv");
	VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

	auto fragShaderCode = HelperFunctions::readShaderFile(SHADERPATH"MaterialScene/scene_frag.spv");
	VkShaderModule fragShaderModule = HelperFunctions::CreateShaderModules(fragShaderCode);

	VkPipelineShaderStageCreateInfo shaderStages[] = 
	{ 
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
	};


	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pRasterizationState = &rasterizerState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.renderPass = graphicsPipeline.renderPass;
	pipelineInfo.layout = graphicsPipeline.pipelineLayout;
	pipelineInfo.subpass = 0;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline.pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline");

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
#pragma endregion
}

void MaterialScene::CreateSyncObjects(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// create semaphores
	renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChain.swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	VkFenceCreateInfo fenceInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create sync objects for a frame");
	}
}

void MaterialScene::CreateObjects()
{
	objects.resize(28);
	int row = 0, col = 0;

	for (int i = 0; i < 28; i++)
	{	
		objects[i] = BasicShapes::createSphere();
		objects[i].setMaterialWithPreset(static_cast<MaterialPresets>(i));

		// move to the next row
		if (i != 0 && i % 9 == 0)
		{
			row++;
			col = 0;
		}

		glm::vec3 pos = glm::vec3(col, 0.0f, -row);

		glm::mat4 model = glm::translate(pos) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));
		objects[i].setModelMatrix(model);
		col++;
	}
}

void MaterialScene::CreateUniforms(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	Camera* camera = Camera::GetCamera();

	VkExtent2D dim = swapChain.swapChainDimensions;
	float aspectRatio = float(dim.width) / float(dim.height);

	light = SpotLight(glm::vec3(1.0, 1.0, 1.0)); 

	uboScene.proj = glm::perspective(glm::radians(camera->GetFOV()), aspectRatio, 0.1f, 1000.0f);
	uboScene.proj[1][1] *= -1;
	glm::mat4 cameraView = camera->GetViewMatrix();
	uboScene.view = cameraView;
	uboScene.lightPos = light.getLightPos();
	uboScene.viewPos = camera->GetCameraPosition();
}

void MaterialScene::UpdateUniforms(uint32_t index)
{
	static Camera* camera = Camera::GetCamera();

	static std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now(),
												 lastTime = std::chrono::steady_clock::now();

	static float timer = 0.0f;

	float dt = 0.0f;
	currentTime = std::chrono::high_resolution_clock::now();
	dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;
	
	if (animate)
		timer += dt;

	// oscillate between -radius and +radius at speed
	float speed = 10.0f;
	float radius = 5.0f;

	// move light in a circle
	glm::vec3 lightPos = glm::vec3(radius * sin(glm::radians(timer * speed)) + 4.0f, 1.0f, -1.0f);
	
	light.setLightPos(lightPos);

	glm::mat4 cameraView = camera->GetViewMatrix();
	uboScene.view = cameraView;
	uboScene.lightPos = lightPos;
	uboScene.viewPos = camera->GetCameraPosition();

	memcpy(graphicsPipeline.uniformBuffers[index].mappedMemory, &uboScene, sizeof(uboScene));
}

void MaterialScene::CreateCommandBuffers()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	commandBuffersList.resize(graphicsPipeline.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffersList.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffersList.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");
}

void MaterialScene::RecordCommandBuffers(uint32_t index)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // defines how we want to use the command buffer
	beginInfo.pInheritanceInfo = nullptr; // only important if we're using secondary command buffers

	if (vkBeginCommandBuffer(commandBuffersList[index], &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to being recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = graphicsPipeline.renderPass;
	renderPassInfo.framebuffer = graphicsPipeline.framebuffers[index];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;

	VkClearValue clearColors[2];
	clearColors[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
	clearColors[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearColors;
	VkDeviceSize offsets[] = { 0 };

	// draw scene
	vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);
	vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
		0, 1, &graphicsPipeline.descriptorSets[index], 0, nullptr);

	DrawScene(commandBuffersList[index], graphicsPipeline.pipelineLayout, true);

	// render UI
	DrawUI(index);
	
	vkCmdEndRenderPass(commandBuffersList[index]);

	if (vkEndCommandBuffer(commandBuffersList[index]) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer");
}

void MaterialScene::DrawUI(uint32_t index)
{
	static Camera* camera = Camera::GetCamera();
	glm::vec3 cameraPos = camera->GetCameraPosition();

	ui->NewUIFrame();

	ui->NewWindow("Application");
	ui->DisplayFPS();
	ui->DrawSliderVec3("Camera Position", &cameraPos, -20.0f, 20.0f);
	ui->EndWindow();

	if (!isCameraMoving)
	{
		static bool showDemoWindow = false;

		if (showDemoWindow)
			ui->ShowDemoWindow();

		ui->NewWindow("Scene Data");
		{
			ui->DrawCheckBox("Show Demo Window", &showDemoWindow);
			ui->DrawSliderVec4("Clear Color", &clearColor, 0.0f, 1.0f);

			// select individual spheres
			if (ui->NewTreeNode("Spheres"))
			{
				for (int i = 0; i < objects.size(); i++)
				{
					bool materialChanged = false;

					if (ui->NewTreeNode((void*)(intptr_t)i, "Sphere %i", i + 1))
					{
						glm::vec3* amb = &objects[i].material->ubo.ambient;
						glm::vec3* dif = &objects[i].material->ubo.diffuse;
						glm::vec3* spc = &objects[i].material->ubo.specular;
						float* shine = &objects[i].material->ubo.shininess;

						if (ui->DrawSliderVec3("Ambient Color", amb, 0.0f, 1.0f))
							materialChanged = true;

						if (ui->DrawSliderVec3("Diffuse Color", dif, 0.0f, 1.0f))
							materialChanged = true;

						if (ui->DrawSliderVec3("Specular Color", spc, 0.0f, 1.0f))
							materialChanged = true;

						if (ui->DrawSliderFloat("Shininess", shine, 1.0f, 256.0f))
							materialChanged = true;

						if (materialChanged)
							objects[i].material->updateMaterial();
							
						ui->EndTreeNode();
					}
				}
				ui->EndTreeNode();
			}
		}
		ui->EndWindow();
	}

	ui->EndFrame();
	ui->RenderFrame(commandBuffersList[index], index);
}
