#include "StencilBuffer.h"

#include "stb_image.h"

StencilBuffer::StencilBuffer(std::string name, const VulkanSwapChain& swapChain)
{
	sceneName = name;


	CreateCommandPool();
	model = new Model("Bananas/", "bananas.obj", commandPool);
	CreateUniforms(swapChain);
	CreateRenderPass(swapChain);
	CreateFramebuffers(swapChain);
	CreateCommandBuffers();
	CreateSyncObjects(swapChain);
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipelines(swapChain);

	RecordScene();
}

StencilBuffer::~StencilBuffer()
{
}

void StencilBuffer::RecordScene()
{
	VkClearValue clearColors[2] = {};
	clearColors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[1].depthStencil = { 1.0f, 0 };

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkDeviceSize offsets[] = { 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = stencilPipeline.scissors.extent;
	renderPassInfo.renderPass = stencilPipeline.renderPass;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearColors;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;

	VkBuffer vertexBuffer = model->getVertexBuffer();
	VkBuffer indexBuffer = model->getIndexBuffer();
	uint32_t size = model->getIndices().size();

	// begin recording command buffers
	for (size_t i = 0; i < commandBuffersList.size(); i++)
	{
		if (vkBeginCommandBuffer(commandBuffersList[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to being recording command buffer!");

		renderPassInfo.framebuffer = stencilPipeline.framebuffers[i];

		// First Render Pass: Render the textured cubes to fill stencil buffer values
		vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind resources
		vkCmdBindVertexBuffers(commandBuffersList[i], 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffersList[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, stencilPipeline.pipelineLayout, 0, 1,
			&stencilPipeline.descriptorSets[i], 0, 0);

		vkCmdPushConstants(commandBuffersList[i], stencilPipeline.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Material), &model->getMaterial());

		// First Pass: fill stencil buffer values
		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, stencilPipeline.pipeline);
		vkCmdDrawIndexed(commandBuffersList[i], size, 1, 0, 0, 0);

		// Second Pass: read from stencil and draw colored outline where values don't match
		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipeline.pipeline);
		vkCmdDrawIndexed(commandBuffersList[i], size, 1, 0, 0, 0);
		
		vkCmdEndRenderPass(commandBuffersList[i]);

		if (vkEndCommandBuffer(commandBuffersList[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
	}
}

void StencilBuffer::DestroyScene(bool isRecreation)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	stencilPipeline.destroyGraphicsPipeline(device);
	colorPipeline.destroyGraphicsPipeline(device);

	if (!isRecreation)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);
	}

	delete model;
}

void StencilBuffer::RecreateScene(const VulkanSwapChain& swapChain)
{
	DestroyScene(true);

	CreateRenderPass(swapChain);
	CreateFramebuffers(swapChain);
	CreateCommandBuffers();
	CreateSyncObjects(swapChain);
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipelines(swapChain);

	RecordScene();
}

VulkanReturnValues StencilBuffer::DrawScene(const VulkanSwapChain& swapChain)
{
	static VulkanDevice* vkDevice = VulkanDevice::GetVulkanDevice();
	static VkDevice device = vkDevice->GetLogicalDevice();
	static VkQueue renderQueue = vkDevice->GetQueues().renderQueue;
	static VkQueue presentQueue = vkDevice->GetQueues().presentQueue;
	
	uint32_t imageIndex;
	stencilPipeline.result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (stencilPipeline.result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;
	}

	else if (stencilPipeline.result != VK_SUCCESS && stencilPipeline.result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}


	// check if a previous frame is using this image
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// mark image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	UpdateUniforms(imageIndex);

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

	if (vkQueueSubmit(renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	VkSwapchainKHR swapChains[] = { swapChain.swapChain };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	stencilPipeline.result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (stencilPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || stencilPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (stencilPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void StencilBuffer::CreateCommandBuffers()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	commandBuffersList.resize(stencilPipeline.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffersList.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffersList.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");
}

void StencilBuffer::CreateRenderPass(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// Color Attachment
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

	// Stencil Attachment
	VkAttachmentDescription depthStencilAttachment;
	depthStencilAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthStencilAttachment.flags = 0;
	depthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference, dsReference;
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	dsReference.attachment = 1;
	dsReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.flags = 0;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pDepthStencilAttachment = &dsReference;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// Subpass dependencies for layout transitions
	VkSubpassDependency dependencies[2] = {};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkAttachmentDescription stencilAttachments[] = { colorAttachment, depthStencilAttachment };

	VkRenderPassCreateInfo stencilRenderPassInfo{};
	stencilRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	stencilRenderPassInfo.attachmentCount = 2;
	stencilRenderPassInfo.pAttachments = stencilAttachments;
	stencilRenderPassInfo.subpassCount = 1;
	stencilRenderPassInfo.pSubpasses = &subpass;
	stencilRenderPassInfo.dependencyCount = 2;
	stencilRenderPassInfo.pDependencies = dependencies;

	stencilPipeline.result = vkCreateRenderPass(device, &stencilRenderPassInfo, nullptr, &stencilPipeline.renderPass);
	if (stencilPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");

	colorPipeline.result = vkCreateRenderPass(device, &stencilRenderPassInfo, nullptr, &colorPipeline.renderPass);
	if (colorPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");

}

void StencilBuffer::CreateFramebuffers(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	stencilPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());
	VkExtent2D dim = swapChain.swapChainDimensions;

	createImage(dim.width, dim.height, 1, VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorPipeline.depthStencilBuffer, colorPipeline.depthStencilBufferMemory);

	createImageView(colorPipeline.depthStencilBuffer, colorPipeline.depthStencilBufferView, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_VIEW_TYPE_2D);

	colorPipeline.isDepthBufferEmpty = false;

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { swapChain.swapChainImageViews[i], colorPipeline.depthStencilBufferView };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = stencilPipeline.renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = dim.width;
		framebufferInfo.height = dim.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &stencilPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}
}

void StencilBuffer::CreateGraphicsPipelines(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// shared resources

	#pragma region VERTEX_INPUT_STATE
	VkVertexInputBindingDescription bindDesc = ModelVertex::getBindingDescription();

	std::array<VkVertexInputAttributeDescription, 3> attrDesc = ModelVertex::getAttributeDescriptions();

	// ** Vertex Input State **
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
#pragma endregion 

	#pragma region VIEWPORT
	// ** Viewport **
	stencilPipeline.viewport.x = 0.0f;
	stencilPipeline.viewport.y = 0.0f;
	stencilPipeline.viewport.width = (float)swapChain.swapChainDimensions.width;
	stencilPipeline.viewport.height = (float)swapChain.swapChainDimensions.height;
	stencilPipeline.viewport.minDepth = 0.0f;
	stencilPipeline.viewport.maxDepth = 1.0f;

	stencilPipeline.scissors.offset = { 0, 0 };
	stencilPipeline.scissors.extent = swapChain.swapChainDimensions;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &stencilPipeline.viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &stencilPipeline.scissors;
#pragma endregion 

	#pragma region RASTERIZER
	// ** Rasterizer **
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable = VK_FALSE;
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
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
		colorBlendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendingAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendingAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
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

	#pragma region STENCIL_STATE

		VkStencilOpState stencilStateFront, stencilStateBack;
		stencilStateFront.reference = 1;
		stencilStateFront.compareMask = 0xff;
		stencilStateFront.writeMask = 0xff;
		stencilStateFront.compareOp = VK_COMPARE_OP_ALWAYS;
		stencilStateFront.failOp = VK_STENCIL_OP_REPLACE;
		stencilStateFront.depthFailOp = VK_STENCIL_OP_REPLACE;
		stencilStateFront.passOp = VK_STENCIL_OP_REPLACE;
		stencilStateBack = stencilStateFront;

	#pragma endregion
		
	#pragma region DEPTH_STENCIL

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = VK_TRUE;
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.stencilTestEnable = VK_TRUE;
		depthStencilInfo.front = stencilStateFront;
		depthStencilInfo.back = stencilStateBack;

	#pragma endregion

	// Pipeline 1: stencil buffer
	#pragma region STENCIL_PIPELINE

#pragma region SHADERS
	auto vertCode = stencilPipeline.readShaderFile(SHADERPATH"Stencil/stencil_buffer_vertex.spv");
	VkShaderModule vertModule = CreateShaderModules(vertCode);

	auto fragCode = stencilPipeline.readShaderFile(SHADERPATH"Stencil/stencil_buffer_fragment.spv");
	VkShaderModule fragModule = CreateShaderModules(fragCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragModule;
	fragShaderStageInfo.pName = "main";


#pragma endregion

	VkPushConstantRange push{};
	push.offset = 0;
	push.size = sizeof(Material);
	push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineShaderStageCreateInfo stencilStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &stencilPipeline.descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &push;

	stencilPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &stencilPipeline.pipelineLayout);
	if (stencilPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout");


	// ** Graphics Pipeline **
	VkGraphicsPipelineCreateInfo stencilPipelineInfo = {};
	stencilPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	stencilPipelineInfo.stageCount = 2;
	stencilPipelineInfo.pStages = stencilStages;
	stencilPipelineInfo.pVertexInputState = &vertexInputInfo;
	stencilPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	stencilPipelineInfo.pViewportState = &viewportState;
	stencilPipelineInfo.pRasterizationState = &rasterizerInfo;
	stencilPipelineInfo.pMultisampleState = &multisamplingInfo;
	stencilPipelineInfo.pColorBlendState = &colorBlendingInfo;
	stencilPipelineInfo.pDepthStencilState = &depthStencilInfo;
	stencilPipelineInfo.layout = stencilPipeline.pipelineLayout;
	stencilPipelineInfo.renderPass = stencilPipeline.renderPass;
	stencilPipelineInfo.subpass = 0;

	stencilPipeline.result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &stencilPipelineInfo, nullptr, &stencilPipeline.pipeline);
	if (stencilPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create stencil pipeline");

	vkDestroyShaderModule(device, vertModule, nullptr);
	vkDestroyShaderModule(device, fragModule, nullptr);

#pragma endregion 

	// pipeline 2: color outline
	#pragma region COLOR_PIPELINE

#pragma region SHADERS
	vertCode = colorPipeline.readShaderFile(SHADERPATH"Stencil/outline_vertex.spv");
	vertModule = CreateShaderModules(vertCode);

	fragCode = colorPipeline.readShaderFile(SHADERPATH"Stencil/outline_fragment.spv");
	fragModule = CreateShaderModules(fragCode);

	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vertModule;
	vertStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = fragModule;
	fragStage.pName = "main";

#pragma endregion

#pragma region STENCIL

	stencilStateFront.compareOp = VK_COMPARE_OP_NOT_EQUAL;
	stencilStateFront.failOp = VK_STENCIL_OP_KEEP;
	stencilStateFront.depthFailOp = VK_STENCIL_OP_KEEP;
	stencilStateFront.passOp = VK_STENCIL_OP_REPLACE;
	stencilStateBack = stencilStateFront;	
	
	depthStencilInfo.front = stencilStateFront;
	depthStencilInfo.back = stencilStateBack;
	depthStencilInfo.depthTestEnable = VK_FALSE;


#pragma endregion

	VkPipelineShaderStageCreateInfo colorStages[] = { vertStage, fragStage };

	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo colorLayoutInfo = {};
	colorLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	colorLayoutInfo.setLayoutCount = 1;
	colorLayoutInfo.pSetLayouts = &colorPipeline.descriptorSetLayout;

	colorPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &colorPipeline.pipelineLayout);
	if (colorPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout");

	// this pipeline is a derivative of the stencil class
	VkGraphicsPipelineCreateInfo colorPipelineInfo = {};
	colorPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	colorPipelineInfo.stageCount = 2;
	colorPipelineInfo.pStages = colorStages;
	colorPipelineInfo.layout = colorPipeline.pipelineLayout;
	colorPipelineInfo.renderPass = colorPipeline.renderPass;
	colorPipelineInfo.pVertexInputState = &vertexInputInfo;
	colorPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	colorPipelineInfo.pViewportState = &viewportState;
	colorPipelineInfo.pRasterizationState = &rasterizerInfo;
	colorPipelineInfo.pMultisampleState = &multisamplingInfo;
	colorPipelineInfo.pColorBlendState = &colorBlendingInfo;
	colorPipelineInfo.pDepthStencilState = &depthStencilInfo;
	colorPipelineInfo.subpass = 0;


	colorPipeline.result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &colorPipelineInfo, nullptr, &colorPipeline.pipeline);
	if (colorPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create color pipeline");

	vkDestroyShaderModule(device, vertModule, nullptr);
	vkDestroyShaderModule(device, fragModule, nullptr);

#pragma endregion
}

void StencilBuffer::CreateDescriptorSets(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	size_t numSamplers = model->getSamplers().size();

#pragma region BINDINGS

	std::vector<VkDescriptorSetLayoutBinding> bindings(1 + numSamplers);

	// create descriptor set for texture for stencil pipeline
	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	for (size_t i = 0; i < numSamplers; i++)
	{
		bindings[i + 1].binding = i + 1;
		bindings[i + 1].descriptorCount = 1;
		bindings[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[i + 1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &stencilPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create cube descriptor set layout!");

	VkDescriptorSetLayoutBinding outlineBinding = {};
	outlineBinding.binding = 0;
	outlineBinding.descriptorCount = 1;
	outlineBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	outlineBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo outlineInfo{};
	outlineInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	outlineInfo.bindingCount = 1;
	outlineInfo.pBindings = &outlineBinding;

	if (vkCreateDescriptorSetLayout(device, &outlineInfo, nullptr, &colorPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create outline descriptor set layout");

#pragma endregion

	size_t swapChainSize = swapChain.swapChainImages.size();

	VkDeviceSize bufferSize = sizeof(UBO);
	stencilPipeline.uniformBuffers.resize(swapChainSize);
	stencilPipeline.uniformBuffersMemory.resize(swapChainSize);
	colorPipeline.uniformBuffers.resize(swapChainSize);
	colorPipeline.uniformBuffersMemory.resize(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stencilPipeline.uniformBuffers[i], stencilPipeline.uniformBuffersMemory[i]);
		
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			colorPipeline.uniformBuffers[i], colorPipeline.uniformBuffersMemory[i]);
	}

#pragma region POOLS_SETS_LAYOUT

	// allocate a descriptor set for each frame
	VkDescriptorPoolSize poolSize[2] = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[0].descriptorCount = static_cast<uint32_t>(swapChainSize);
	poolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[1].descriptorCount = static_cast<uint32_t>(swapChainSize);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(swapChainSize);

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &stencilPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	VkDescriptorPoolCreateInfo outlinePoolInfo = {};
	outlinePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	outlinePoolInfo.poolSizeCount = 1;
	outlinePoolInfo.pPoolSizes = &poolSize[1];
	outlinePoolInfo.maxSets = static_cast<uint32_t>(swapChainSize);

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &colorPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	std::vector<VkDescriptorSetLayout> layout(swapChainSize, stencilPipeline.descriptorSetLayout);
	std::vector<VkDescriptorSetLayout> outlineLayout(swapChainSize, colorPipeline.descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = stencilPipeline.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	allocInfo.pSetLayouts = layout.data();
	stencilPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &allocInfo, stencilPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

	VkDescriptorSetAllocateInfo outlineAllocInfo = {};
	outlineAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	outlineAllocInfo.descriptorPool = colorPipeline.descriptorPool;
	outlineAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	outlineAllocInfo.pSetLayouts = outlineLayout.data();
	colorPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &outlineAllocInfo, colorPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate outline descriptor sets");

#pragma endregion

#pragma region WRITES

	std::vector<VkWriteDescriptorSet> descriptorWrites(1 + numSamplers);
	const std::vector<VkImageView> imageViews = model->getImageViews();
	const std::vector<VkSampler> samplers = model->getSamplers();

	// UBO and samplers for stencil pipeline
	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = stencilPipeline.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBO);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = stencilPipeline.descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		for (size_t j = 0; j < numSamplers; j++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = imageViews[j];
			imageInfo.sampler = samplers[j];

			descriptorWrites[j + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[j + 1].dstSet = stencilPipeline.descriptorSets[i];
			descriptorWrites[j + 1].dstBinding = j + 1;
			descriptorWrites[j + 1].dstArrayElement = 0;
			descriptorWrites[j + 1].descriptorCount = 1;
			descriptorWrites[j + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[j + 1].pImageInfo = &imageInfo; // used for descriptors that refer to image data
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	// UBO for outline pipeline
	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = colorPipeline.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBO);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = colorPipeline.descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}

#pragma endregion

	stencilPipeline.isDescriptorPoolEmpty = false;
	colorPipeline.isDescriptorPoolEmpty = false;

}

void StencilBuffer::CreateSyncObjects(const VulkanSwapChain& swapChain)
{
	renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChain.swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	VkFenceCreateInfo fenceInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create sync objects for a frame");
	}
}

void StencilBuffer::CreateUniforms(const VulkanSwapChain& swapChain)
{
	Camera* camera = Camera::GetCamera();
	ubo.model = glm::mat4(1.0f);
	ubo.view = camera->GetViewMatrix();
	ubo.proj = glm::perspective(glm::radians(90.0f), float(swapChain.swapChainDimensions.width / swapChain.swapChainDimensions.height), 0.01f, 1000.0f);
	ubo.proj[1][1] *= -1;
	ubo.outlineWidth = 0.025f;
}

void StencilBuffer::UpdateUniforms(uint32_t currentFrame)
{
	static Camera* camera = Camera::GetCamera();
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	ubo.view = camera->GetViewMatrix();

	void* data;
	vkMapMemory(device, stencilPipeline.uniformBuffersMemory[currentFrame], 0, sizeof(UBO), 0, &data);
	memcpy(data, &ubo, sizeof(UBO));
	vkUnmapMemory(device, stencilPipeline.uniformBuffersMemory[currentFrame]);

	vkMapMemory(device, colorPipeline.uniformBuffersMemory[currentFrame], 0, sizeof(UBO), 0, &data);
	memcpy(data, &ubo, sizeof(UBO));
	vkUnmapMemory(device, colorPipeline.uniformBuffersMemory[currentFrame]);
}
