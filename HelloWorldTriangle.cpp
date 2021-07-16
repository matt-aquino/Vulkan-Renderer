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

#include "HelloWorldTriangle.h"

HelloWorldTriangle::HelloWorldTriangle()
{
}

HelloWorldTriangle::HelloWorldTriangle(std::string name, const VkInstance& instance,
	const VkSurfaceKHR& appSurface, const VulkanSwapChain& swapChain)
{
	sceneName = name;
	device = device->GetVulkanDevice(instance, appSurface);

	CreateRenderPass(swapChain);
	CreateGraphicsPipeline(swapChain);
	CreateFramebuffers(swapChain);
	CreateSyncObjects(swapChain);
	CreateCommandPool();
	CreateVertexBuffer();
	CreateScene();
}

void HelloWorldTriangle::CreateScene() 
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
		renderPassInfo.renderPass = graphicsPipeline.renderPass;
		renderPassInfo.framebuffer = graphicsPipeline.framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);

		VkBuffer buffers[] = { graphicsPipeline.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffersList[i], 0, 1, buffers, offsets);

		vkCmdDraw(commandBuffersList[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

		// params: command buffer, vertex count, instanceCount (for instanced rendering), first vertex, first instance
		vkCmdEndRenderPass(commandBuffersList[i]);

		if (vkEndCommandBuffer(commandBuffersList[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
	}
	
}

void HelloWorldTriangle::RecreateScene(const VulkanSwapChain& swapChain)
{
	// destroy the whole pipeline and recreate for new swap chain
	for (size_t i = 0; i < graphicsPipeline.framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device->logicalDevice, graphicsPipeline.framebuffers[i], nullptr);
	}

	vkFreeCommandBuffers(device->logicalDevice, commandPool, static_cast<uint32_t>(commandBuffersList.size()), commandBuffersList.data());

	vkDestroyPipeline(device->logicalDevice, graphicsPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(device->logicalDevice, graphicsPipeline.pipelineLayout, nullptr);
	vkDestroyRenderPass(device->logicalDevice, graphicsPipeline.renderPass, nullptr);

	CreateRenderPass(swapChain);
	CreateGraphicsPipeline(swapChain);
	CreateFramebuffers(swapChain);
	CreateVertexBuffer();
	CreateCommandPool();
	CreateScene();
}

VulkanReturnValues HelloWorldTriangle::RunScene(const VulkanSwapChain& swapChain)
{
	uint32_t imageIndex;
	graphicsPipeline.result = vkAcquireNextImageKHR(device->logicalDevice, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
		vkWaitForFences(device->logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

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
	
	vkResetFences(device->logicalDevice, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(device->GetQueues().renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
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

	graphicsPipeline.result = vkQueuePresentKHR(device->GetQueues().presentQueue, &presentInfo);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || graphicsPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void HelloWorldTriangle::DestroyScene() 
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device->logicalDevice, renderCompleteSemaphores[i], nullptr);
		vkDestroySemaphore(device->logicalDevice, presentCompleteSemaphores[i], nullptr);
		vkDestroyFence(device->logicalDevice, inFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(device->logicalDevice, commandPool, nullptr);

	for (VkFramebuffer fb : graphicsPipeline.framebuffers)
	{
		vkDestroyFramebuffer(device->logicalDevice, fb, nullptr);
	}

	for (VkShaderModule module : graphicsPipeline.sceneShaderModules)
	{
		vkDestroyShaderModule(device->logicalDevice, module, nullptr);
	}

	vkDestroyPipeline(device->logicalDevice, graphicsPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(device->logicalDevice, graphicsPipeline.pipelineLayout, nullptr);
	vkDestroyRenderPass(device->logicalDevice, graphicsPipeline.renderPass, nullptr);
	vkDestroyBuffer(device->logicalDevice, graphicsPipeline.vertexBuffer, nullptr);
	vkFreeMemory(device->logicalDevice, graphicsPipeline.vertexBufferMemory, nullptr);
}

void HelloWorldTriangle::CreateGraphicsPipeline(const VulkanSwapChain& swapChain)
{
	// ** Shaders **
	auto vertShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Triangle/basic_triangle_vertex.spv");
	CreateShaderModules(vertShaderCode);

	auto fragShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Triangle/basic_triangle_fragment.spv");
	CreateShaderModules(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//vertShaderStageInfo.pNext = nullptr;
	//vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = graphicsPipeline.sceneShaderModules[0];
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//fragShaderStageInfo.pNext = nullptr;
	//fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = graphicsPipeline.sceneShaderModules[1];
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> attributeDescription = Vertex::getAttributeDescriptions();

	// ** Vertex Input State **
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	//inputAssemblyInfo.pNext = nullptr;
	//inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

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
	//viewportState.pNext = nullptr;
	//viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &graphicsPipeline.viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &graphicsPipeline.scissors;

	// ** Rasterizer **
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	//rasterizerInfo.pNext = nullptr;
	//rasterizerInfo.flags = 0;
	rasterizerInfo.depthClampEnable = VK_FALSE; // setting to true is useful for special cases like shadow maps; requires a GPU feature to be enabled
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; // setting to true discards all output to the framebuffer
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL; // options: Line - Point - Fill; anything but fill requires GPU feature
	rasterizerInfo.lineWidth = 1.0f; // increases/decreases number of fragments used in line thickness; max number depends on hardware and wideLines GPU feature
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT; // back face culling
	rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	// ** Multisampling **
	VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	//multisamplingInfo.pNext = nullptr;
	//multisamplingInfo.flags = 0;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;

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
	//colorBlendingInfo.pNext = nullptr;
	//colorBlendingInfo.flags = 0;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = 1;
	colorBlendingInfo.pAttachments = &colorBlendingAttachment;
	colorBlendingInfo.blendConstants[0] = 0.0f;
	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;

	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//layoutInfo.pNext = nullptr;
	//layoutInfo.flags = 0;
	layoutInfo.setLayoutCount = 0;
	layoutInfo.pSetLayouts = nullptr;
	layoutInfo.pushConstantRangeCount = 0; // push constants are the uniform variables used in our shaders.
	layoutInfo.pPushConstantRanges = nullptr;

	graphicsPipeline.result = vkCreatePipelineLayout(device->logicalDevice, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout");


	// ** Graphics Pipeline **
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = 2;
	graphicsPipelineInfo.pStages = shaderStages;
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pTessellationState = nullptr;
	graphicsPipelineInfo.pViewportState = &viewportState;
	graphicsPipelineInfo.pRasterizationState = &rasterizerInfo; // rasterizer
	graphicsPipelineInfo.pMultisampleState = &multisamplingInfo; // multisampling
	graphicsPipelineInfo.pDepthStencilState = nullptr; // depth/stencil buffers
	graphicsPipelineInfo.pColorBlendState = &colorBlendingInfo; // color blending
	//graphicsPipelineInfo.pDynamicState = nullptr; // dynamic states used for changing specific aspects of the pipeline at draw time
	graphicsPipelineInfo.layout = graphicsPipeline.pipelineLayout;
	graphicsPipelineInfo.renderPass = graphicsPipeline.renderPass;
	graphicsPipelineInfo.subpass = 0;
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // use this to create new pipeline from an existing pipeline
	graphicsPipelineInfo.basePipelineIndex = -1;

	// this is breaking...
	graphicsPipeline.result = vkCreateGraphicsPipelines(device->logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline.pipeline);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline");
}

void HelloWorldTriangle::CreateRenderPass(const VulkanSwapChain& swapChain)
{
	VkAttachmentDescription colorAttachment;
	colorAttachment.format = swapChain.swapChainImageFormat;
	colorAttachment.flags = 0;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear existing contents
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store contents in memory for use later
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // existing contents are undefined; we don't care about them
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // we don't care what the previous layout was, since we'll clear it anyway
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // image will be presented in the swap chain

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0; // we have a single attachment, so its index is 0
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // we will use this image as a color buffer

	VkSubpassDescription subpass;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.pResolveAttachments = nullptr;

	VkSubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;	

	graphicsPipeline.result = vkCreateRenderPass(device->logicalDevice, &renderPassInfo, nullptr, &graphicsPipeline.renderPass);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}

void HelloWorldTriangle::CreateFramebuffers(const VulkanSwapChain& swapChain)
{
	graphicsPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { swapChain.swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = graphicsPipeline.renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChain.swapChainDimensions.width;
		framebufferInfo.height = swapChain.swapChainDimensions.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device->logicalDevice, &framebufferInfo, nullptr, &graphicsPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}
}


void HelloWorldTriangle::CreateShaderModules(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//createInfo.pNext = nullptr;
	//createInfo.flags = 0;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	graphicsPipeline.result = vkCreateShaderModule(device->logicalDevice, &createInfo, nullptr, &shaderModule);
	
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module");

	graphicsPipeline.sceneShaderModules.push_back(shaderModule);
}

void HelloWorldTriangle::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = device->GetFamilyIndices().graphicsFamily.value();

	if (vkCreateCommandPool(device->logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool");

	commandBuffersList.resize(graphicsPipeline.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffersList.size();

	if (vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, commandBuffersList.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");
}

void HelloWorldTriangle::CreateSyncObjects(const VulkanSwapChain& swapChain)
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
		if (vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device->logicalDevice, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device->logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create sync objects for a frame");
	}


}

void HelloWorldTriangle::CreateVertexBuffer()
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(vertices[0]) * vertices.size();
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffers can be shared between queue families just like images

	graphicsPipeline.result = vkCreateBuffer(device->logicalDevice, &bufferInfo, nullptr, &graphicsPipeline.vertexBuffer);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer");

	// assign memory to buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device->logicalDevice, graphicsPipeline.vertexBuffer, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &memProperties);

	uint32_t typeIndex = 0;
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			typeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = typeIndex;
	graphicsPipeline.result = vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &graphicsPipeline.vertexBufferMemory);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate vertex buffer memory");

	vkBindBufferMemory(device->logicalDevice, graphicsPipeline.vertexBuffer, graphicsPipeline.vertexBufferMemory, 0); // if offset is not 0, it MUST be visible by memRequirements.alignment

	void* data;
	vkMapMemory(device->logicalDevice, graphicsPipeline.vertexBufferMemory, 0, bufferInfo.size, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(device->logicalDevice, graphicsPipeline.vertexBufferMemory);

}