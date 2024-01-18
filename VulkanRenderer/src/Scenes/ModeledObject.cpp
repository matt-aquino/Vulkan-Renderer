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

#include "ModeledObject.h"
ModeledObject::ModeledObject(std::string name, const VulkanSwapChain& swapChain)
{
	sceneName = name;

	object = ModelLoader::loadModel("ZeldaChest", "Medium_Chest.obj");

	CreateRenderPass(swapChain);
	CreateUniforms(swapChain);
	CreateFramebuffer(swapChain);
	CreateCommandBuffers();
	CreateSyncObjects(swapChain);
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);
	
	RecordScene();
}

ModeledObject::~ModeledObject()
{
	DestroyScene(false);
}

void ModeledObject::RecordScene()
{
	// begin recording command buffers
	
	for (size_t i = 0; i < commandBuffersList.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // defines how we want to use the command buffer
		beginInfo.pInheritanceInfo = nullptr; // only important if we're using secondary command buffers

		if (vkBeginCommandBuffer(commandBuffersList[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to being recording command buffer!");

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;

		VkClearValue clearColors[2] = {};
		clearColors[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clearColors[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;

		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 
			0, 1, &graphicsPipeline.descriptorSets[i], 0, nullptr);
		
		vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		DrawScene(commandBuffersList[i], graphicsPipeline.pipelineLayout, true);

		vkCmdEndRenderPass(commandBuffersList[i]);

		if (vkEndCommandBuffer(commandBuffersList[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
	}
}

void ModeledObject::RecreateScene(const VulkanSwapChain& swapChain)
{
	DestroyScene(true);

	// recreate the scene
	CreateRenderPass(swapChain);
	CreateUniforms(swapChain);
	CreateFramebuffer(swapChain);
	CreateCommandBuffers();
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);

	RecordScene();
}

void ModeledObject::DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
	object->draw(commandBuffer, pipelineLayout, useMaterial);
}

VulkanReturnValues ModeledObject::PresentScene(const VulkanSwapChain& swapChain)
{
	static auto queues = VulkanDevice::GetVulkanDevice()->GetQueues();

	// Our state
	static bool show_demo_window = true;
	static bool show_another_window = false;
	uint32_t imageIndex;

	VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain.swapChain,
		UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire swap chain image");

	UpdateUniforms(imageIndex); // update matrices as needed

	// check if a previous frame is using this image
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

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

	/*
	********* RENDERING *************
	*/
	VkSemaphore signalSemaphores[] = { renderCompleteSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(queues.renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	/*
	********* PRESENTATION *************
	*/
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(queues.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void ModeledObject::DestroyScene(bool isRecreation)
{
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	graphicsPipeline.destroyGraphicsPipeline(logicalDevice);

	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(logicalDevice, framebuffers[i], nullptr);
	}
	vkDestroyImage(logicalDevice, depthFBAttachment.image, nullptr);
	vkDestroyImageView(logicalDevice, depthFBAttachment.imageView, nullptr);
	vkFreeMemory(logicalDevice, depthFBAttachment.memory, nullptr);

	uint32_t size = static_cast<uint32_t>(commandBuffersList.size());
	vkFreeCommandBuffers(logicalDevice, commandPool, size, commandBuffersList.data());

	// these only NEED to be deleted once cleanup happens
	if (!isRecreation)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(logicalDevice, renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(logicalDevice, presentCompleteSemaphores[i], nullptr);
			vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
		}	

		delete object;
	}
}

void ModeledObject::HandleKeyboardInput(const uint8_t* keystates, float dt)
{
	Camera* camera = Camera::GetCamera();

	// camera movement
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

void ModeledObject::HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY)
{
	static Camera* camera = Camera::GetCamera();

	static float deltaX = 0.0f;
	static float deltaY = 0.0f;

	isCameraMoving = ((buttons & SDL_BUTTON_RMASK) != 0);

	if (isCameraMoving)
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);

		float sensivity = 0.1f;
		deltaX = x * sensivity;
		deltaY = y * sensivity;

		camera->RotateCamera(deltaX, deltaY);
	}

	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

void ModeledObject::CreateGraphicsPipeline(const VulkanSwapChain& swapChain)
{
#pragma region SHADERS
	auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"Model/model_vertex_shader.spv");
	VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

	auto fragShaderCode = HelperFunctions::readShaderFile(SHADERPATH"Model/model_fragment_shader.spv");
	VkShaderModule fragShaderModule = HelperFunctions::CreateShaderModules(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

#pragma endregion 

#pragma region VERTEX_INPUT_STATE

	VkVertexInputBindingDescription bindingDescription = ModelVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescription = ModelVertex::getAttributeDescriptions();
	
	// ** Vertex Input State **
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
#pragma endregion 

#pragma region VIEWPORT
	// ** Viewport **
	graphicsPipeline.viewport.x = 0.0f;
	graphicsPipeline.viewport.y = 0.0f;
	graphicsPipeline.viewport.width = (float)swapChain.swapChainDimensions.width;
	graphicsPipeline.viewport.height = (float)swapChain.swapChainDimensions.height;
	graphicsPipeline.viewport.minDepth = 0.0f;
	graphicsPipeline.viewport.maxDepth = 1.0f;

	graphicsPipeline.scissors.offset = { 0, 0 };
	graphicsPipeline.scissors.extent = swapChain.swapChainDimensions;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &graphicsPipeline.viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &graphicsPipeline.scissors;
#pragma endregion 

#pragma region RASTERIZER
	// ** Rasterizer **
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable = VK_FALSE; 
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; 
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL; 
	rasterizerInfo.lineWidth = 1.0f; 
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT; 
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;
#pragma endregion 

#pragma region MULTISAMPLING
	// ** Multisampling **
	VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;
#pragma endregion 

#pragma region COLOR_BLENDING
	// ** Color Blending **
	VkPipelineColorBlendAttachmentState colorBlendingAttachment;
	colorBlendingAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendingAttachment.blendEnable = VK_FALSE;
	colorBlendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendingAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendingAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendingAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = 1;
	colorBlendingInfo.pAttachments = &colorBlendingAttachment;
	colorBlendingInfo.blendConstants[0] = 0.0f;
	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;
#pragma endregion 

#pragma region DEPTH_STENCIL
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;
#pragma endregion

	VkDescriptorSetLayout layouts[] = { graphicsPipeline.descriptorSetLayout, object->meshes[0]->descriptorSetLayout, object->meshes[0]->material->descriptorSetLayout};
	//VkDescriptorSetLayout layouts[] = { graphicsPipeline.descriptorSetLayout, object->meshes[0]->material->descriptorSetLayout};
	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pushConstantRangeCount = 0;
	layoutInfo.pPushConstantRanges = nullptr;
	layoutInfo.setLayoutCount = 3;
	layoutInfo.pSetLayouts = layouts;

	if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout");


	// ** Graphics Pipeline **
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = 2;
	graphicsPipelineInfo.pStages = shaderStages;
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pViewportState = &viewportState;
	graphicsPipelineInfo.pRasterizationState = &rasterizerInfo; // rasterizer
	graphicsPipelineInfo.pMultisampleState = &multisamplingInfo; // multisampling
	graphicsPipelineInfo.pColorBlendState = &colorBlendingInfo; // color blending
	graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
	graphicsPipelineInfo.layout = graphicsPipeline.pipelineLayout;
	graphicsPipelineInfo.renderPass = renderPass;
	graphicsPipelineInfo.subpass = 0;
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // use this to create new pipeline from an existing pipeline
	graphicsPipelineInfo.basePipelineIndex = -1;

	
	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline.pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline");

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void ModeledObject::CreateRenderPass(const VulkanSwapChain& swapChain)
{
	VkAttachmentDescription colorAttachment;
	colorAttachment.format = swapChain.swapChainImageFormat;
	colorAttachment.flags = 0;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0; 
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment;
	depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthAttachment.flags = 0;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference;
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}

void ModeledObject::CreateFramebuffer(const VulkanSwapChain& swapChain)
{
	framebuffers.resize(swapChain.swapChainImageViews.size());
	VkExtent2D dim = swapChain.swapChainDimensions;

	HelperFunctions::createImage(dim.width, dim.height, 1, 1, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthFBAttachment.image, depthFBAttachment.memory);

	HelperFunctions::createImageView(depthFBAttachment.image, depthFBAttachment.imageView, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { swapChain.swapChainImageViews[i], depthFBAttachment.imageView};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = dim.width;
		framebufferInfo.height = dim.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}

}

void ModeledObject::CreateSyncObjects(const VulkanSwapChain& swapChain)
{
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
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create sync objects for a frame");
	}
}

void ModeledObject::CreateUniforms(const VulkanSwapChain& swapChain)
{
	Camera* const camera = Camera::GetCamera();
	ubo.cameraPosition = camera->GetCameraPosition();
	ubo.model = glm::mat4(1.0f);
	ubo.view = camera->GetViewMatrix();
	ubo.projection = glm::perspective(glm::radians(camera->GetFOV()), float(swapChain.swapChainDimensions.width / swapChain.swapChainDimensions.height), 0.1f, 1000.0f);
	ubo.projection[1][1] *= -1;	
}

void ModeledObject::UpdateUniforms(uint32_t index)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count(); // grab time since start of application
	static float angle = 0.0f;
	angle = glm::mod(time * 45.0f, 360.0f);
	static Camera* camera = Camera::GetCamera();
	printf("\rAngle: %f", angle);

	ubo.model = glm::rotate(glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.cameraPosition = camera->GetCameraPosition();
	ubo.view = camera->GetViewMatrix();

	void* data;
	vkMapMemory(logicalDevice, graphicsPipeline.uniformBuffers[index].bufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(UniformBufferObject));
	vkUnmapMemory(logicalDevice, graphicsPipeline.uniformBuffers[index].bufferMemory);
}

void ModeledObject::CreateCommandBuffers()
{
	commandBuffersList.resize(framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffersList.size();

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffersList.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");
}

void ModeledObject::CreateDescriptorSets(const VulkanSwapChain& swapChain)
{
	std::vector<VkImageView> imageViews;
	std::vector<VkSampler> samplers;
	size_t swapChainSize = swapChain.swapChainImages.size();
	uint32_t descriptorCount = static_cast<uint32_t>(swapChainSize);

#pragma region DESCRIPTOR_POOL
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = descriptorCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = descriptorCount;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	graphicsPipeline.isDescriptorPoolEmpty = false;
#pragma endregion

#pragma region BINDINGS
	
	// create binding of UBO
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
#pragma endregion


	// create uniform buffers for graphics pipeline
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	graphicsPipeline.uniformBuffers.resize(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++)
		HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			graphicsPipeline.uniformBuffers[i].buffer, graphicsPipeline.uniformBuffers[i].bufferMemory);
	
#pragma region POOLS_SET_LAYOUT
	

	std::vector<VkDescriptorSetLayout> layout(swapChainSize, graphicsPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = descriptorCount;
	allocInfo.pSetLayouts = layout.data();
	graphicsPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

#pragma endregion

#pragma region WRITES
	VkWriteDescriptorSet descriptorWrite = {};

	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = graphicsPipeline.uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = graphicsPipeline.descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;

		
		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}
#pragma endregion
	
}
