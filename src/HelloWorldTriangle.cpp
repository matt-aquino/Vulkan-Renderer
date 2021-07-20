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

	CreateUniforms(swapChain);
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
		vkCmdPushConstants(commandBuffersList[i], graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &graphicsPipeline.pushConstant);
		vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 0, 1, &graphicsPipeline.descriptorSets[i], 0, nullptr);

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
	
	size_t uniformBufferSize = graphicsPipeline.uniformBuffers.size();
	for (size_t i = 0; i < uniformBufferSize; i++)
	{
		vkDestroyBuffer(device->logicalDevice, graphicsPipeline.uniformBuffers[i], nullptr);
		vkFreeMemory(device->logicalDevice, graphicsPipeline.uniformBuffersMemory[i], nullptr);
	}
	vkDestroyDescriptorPool(device->logicalDevice, graphicsPipeline.descriptorPool, nullptr);

	CreateUniforms(swapChain);
	CreateRenderPass(swapChain);
	CreateGraphicsPipeline(swapChain);
	CreateFramebuffers(swapChain);
	CreateVertexBuffer();
	CreateCommandPool();
	CreateScene();
}

VulkanReturnValues HelloWorldTriangle::RunScene(const VulkanSwapChain& swapChain)
{
	/* Scene Main Loop
	*
	*  1. Acquire image from swap chain
	*  2. Select appropriate command buffer, execute it
	*  3. Return image to swap chain for presentation
	*
	*/

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

	UpdateUniforms(imageIndex); // update matrices as needed

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

void HelloWorldTriangle::CreateUniforms(const VulkanSwapChain& swapChain)
{
	// create uniform matrices
	graphicsPipeline.ubo.model = glm::mat4(1.0f);
	graphicsPipeline.ubo.view = glm::translate(glm::mat4(1.0f), cameraPosition);
	graphicsPipeline.pushConstant.projectionMatrix = glm::perspective(glm::radians(90.0f), graphicsPipeline.viewport.width / graphicsPipeline.viewport.height, 0.1f, 100.0f);

	// create descriptor sets for UBO
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

	if (vkCreateDescriptorSetLayout(device->logicalDevice, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");

	size_t swapChainSize = swapChain.swapChainImages.size();
	
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	graphicsPipeline.uniformBuffers.resize(swapChainSize);
	graphicsPipeline.uniformBuffersMemory.resize(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			graphicsPipeline.uniformBuffers[i], graphicsPipeline.uniformBuffersMemory[i]);
	}

	// allocate a descriptor set for each frame
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(swapChainSize);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(swapChainSize);

	if (vkCreateDescriptorPool(device->logicalDevice, &poolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	std::vector<VkDescriptorSetLayout> layout(swapChainSize, graphicsPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	allocInfo.pSetLayouts = layout.data();
	graphicsPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = graphicsPipeline.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = graphicsPipeline.descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // used for descriptors that refer to image data
		descriptorWrite.pTexelBufferView = nullptr; // used for descriptors that refer to buffer views
		vkUpdateDescriptorSets(device->logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}
}

void HelloWorldTriangle::UpdateUniforms(uint32_t currentImage)
{
	// Calculate time since last frame
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count(); // grab time since start of application

	graphicsPipeline.ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	graphicsPipeline.ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f));
	graphicsPipeline.pushConstant.projectionMatrix = glm::perspective(glm::radians(90.0f), graphicsPipeline.viewport.width / graphicsPipeline.viewport.height, 0.1f, 100.0f);
	
	void* data;
	vkMapMemory(device->logicalDevice, graphicsPipeline.uniformBuffersMemory[currentImage], 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &graphicsPipeline.ubo, sizeof(UniformBufferObject));
	vkUnmapMemory(device->logicalDevice, graphicsPipeline.uniformBuffersMemory[currentImage]);
	
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


	size_t uniformBufferSize = graphicsPipeline.uniformBuffers.size();
	for (size_t i = 0; i < uniformBufferSize; i++)
	{
		vkDestroyBuffer(device->logicalDevice, graphicsPipeline.uniformBuffers[i], nullptr);
		vkFreeMemory(device->logicalDevice, graphicsPipeline.uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(device->logicalDevice, graphicsPipeline.descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device->logicalDevice, graphicsPipeline.descriptorSetLayout, nullptr);
	vkDestroyPipeline(device->logicalDevice, graphicsPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(device->logicalDevice, graphicsPipeline.pipelineLayout, nullptr);
	vkDestroyRenderPass(device->logicalDevice, graphicsPipeline.renderPass, nullptr);
	vkDestroyBuffer(device->logicalDevice, graphicsPipeline.vertexBuffer, nullptr);
	vkFreeMemory(device->logicalDevice, graphicsPipeline.vertexBufferMemory, nullptr);
}

void HelloWorldTriangle::CreateGraphicsPipeline(const VulkanSwapChain& swapChain)
{
#pragma region SHADERS
	auto vertShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Triangle/basic_triangle_vertex.spv");
	VkShaderModule vertShaderModule = CreateShaderModules(vertShaderCode);

	auto fragShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Triangle/basic_triangle_fragment.spv");
	VkShaderModule fragShaderModule = CreateShaderModules(fragShaderCode);

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
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> attributeDescription = Vertex::getAttributeDescriptions();

#pragma endregion 

#pragma region VERTEX_INPUT_STATE
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
	rasterizerInfo.depthClampEnable = VK_FALSE; // setting to true is useful for special cases like shadow maps; requires a GPU feature to be enabled
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; // setting to true discards all output to the framebuffer
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL; // options: Line - Point - Fill; anything but fill requires GPU feature
	rasterizerInfo.lineWidth = 1.0f; // increases/decreases number of fragments used in line thickness; max number depends on hardware and wideLines GPU feature
	rasterizerInfo.cullMode = VK_CULL_MODE_NONE; // no face culling
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

#pragma region PUSH_CONSTANTS
	// Push Constants - uniform variables that we can access in our shaders
	VkPushConstantRange pushConstants;
	pushConstants.offset = 0;
	pushConstants.size = sizeof(PushConstants);
	pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
#pragma endregion 

#pragma region DYNAMIC_STATE
// to be filled eventually
#pragma endregion

	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pushConstantRangeCount = 1; // push constants are an efficient way to pass data to shader, but are limited in size
	layoutInfo.pPushConstantRanges = &pushConstants;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &graphicsPipeline.descriptorSetLayout;

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

	vkDestroyShaderModule(device->logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(device->logicalDevice, fragShaderModule, nullptr);
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
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device->logicalDevice, stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, graphicsPipeline.vertexBuffer, graphicsPipeline.vertexBufferMemory);

	copyBuffer(stagingBuffer, graphicsPipeline.vertexBuffer, bufferSize, device->GetQueues().renderQueue);

	vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(device->logicalDevice, stagingBufferMemory, nullptr);
}

void HelloWorldTriangle::createImages(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{

}