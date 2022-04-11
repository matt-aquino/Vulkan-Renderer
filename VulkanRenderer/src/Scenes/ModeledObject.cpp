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
#include "Renderer/Renderer.h"

ModeledObject::ModeledObject(std::string name, const VulkanSwapChain& swapChain)
{
	sceneName = name;

	CreateCommandPool();
	object = new Model("ZeldaChest/", "Medium_Chest.obj", commandPool);

	CreateRenderPass(swapChain);
	CreateUniforms(swapChain);
	CreateFramebuffer(swapChain);
	CreateCommandBuffers();
	CreateSyncObjects(swapChain);
	CreateDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);
	
	RecordScene();
}


ModeledObject::ModeledObject()
{
	sceneName = "";
	object = nullptr;
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
		renderPassInfo.renderPass = graphicsPipeline.renderPass;
		renderPassInfo.framebuffer = graphicsPipeline.framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;

		VkClearValue clearColors[2] = {};
		clearColors[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clearColors[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;

		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);
		
		// render models
		{
			vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkDeviceSize offsets[] = { 0 };
			for (Mesh mesh : object->meshes)
			{
				// bind vertex, index, and uniform buffers
				vkCmdBindVertexBuffers(commandBuffersList[i], 0, 1, &mesh.vertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(commandBuffersList[i], mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
					0, 1, &graphicsPipeline.descriptorSets[i], 0, nullptr);

				materialConstants.ambient = object->materials[mesh.material_ID].ambient;
				materialConstants.diffuse = object->materials[mesh.material_ID].diffuse;
				materialConstants.specular = object->materials[mesh.material_ID].specular;
				materialConstants.shininess = object->materials[mesh.material_ID].specularExponent;

				// push material constants
				vkCmdPushConstants(commandBuffersList[i], graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MaterialConstants), &materialConstants);

				// draw from index buffer
				vkCmdDrawIndexed(commandBuffersList[i], static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(commandBuffersList[i]);
		}

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

VulkanReturnValues ModeledObject::DrawScene(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// Our state
	static bool show_demo_window = true;
	static bool show_another_window = false;
	uint32_t imageIndex;

	graphicsPipeline.result = vkAcquireNextImageKHR(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), swapChain.swapChain, 
		UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS && graphicsPipeline.result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire swap chain image");

	UpdateUniforms(imageIndex); // update matrices as needed

	// check if a previous frame is using this image
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

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

	vkResetFences(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
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

	graphicsPipeline.result = vkQueuePresentKHR(VulkanDevice::GetVulkanDevice()->GetQueues().presentQueue, &presentInfo);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || graphicsPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void ModeledObject::DestroyScene(bool isRecreation)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	graphicsPipeline.destroyGraphicsPipeline(device);

	uint32_t size = static_cast<uint32_t>(commandBuffersList.size());
	vkFreeCommandBuffers(device, commandPool, size, commandBuffersList.data());

	// these only NEED to be deleted once cleanup happens
	if (!isRecreation)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}	

		vkDestroyCommandPool(device, commandPool, nullptr);
		delete object;
	}
}

void ModeledObject::CreateGraphicsPipeline(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

#pragma region SHADERS
	auto vertShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Model/model_vertex_shader.spv");
	VkShaderModule vertShaderModule = CreateShaderModules(vertShaderCode);

	auto fragShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Model/model_fragment_shader.spv");
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
	graphicsPipeline.isDepthBufferEmpty = false;
#pragma endregion

#pragma region PUSH_CONSTANTS
	// push constants for object material
	VkPushConstantRange fragPush;
	fragPush.offset = 0;
	fragPush.size = sizeof(MaterialConstants);
	fragPush.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
#pragma endregion 

#pragma region DYNAMIC_STATE
	// to be filled eventually
#pragma endregion

	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &fragPush;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &graphicsPipeline.descriptorSetLayout;

	graphicsPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout);
	if (graphicsPipeline.result != VK_SUCCESS)
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
	graphicsPipelineInfo.renderPass = graphicsPipeline.renderPass;
	graphicsPipelineInfo.subpass = 0;
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // use this to create new pipeline from an existing pipeline
	graphicsPipelineInfo.basePipelineIndex = -1;

	
	graphicsPipeline.result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline.pipeline);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline");

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
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

	graphicsPipeline.result = vkCreateRenderPass(VulkanDevice::GetVulkanDevice()->GetLogicalDevice(), &renderPassInfo, nullptr, &graphicsPipeline.renderPass);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}

void ModeledObject::CreateFramebuffer(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	graphicsPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());
	VkExtent2D dim = swapChain.swapChainDimensions;


	createImage(dim.width, dim.height, 1, VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferMemory);

	createImageView(graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferView, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { swapChain.swapChainImageViews[i], graphicsPipeline.depthStencilBufferView};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = graphicsPipeline.renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = dim.width;
		framebufferInfo.height = dim.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &graphicsPipeline.framebuffers[i]) != VK_SUCCESS)
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

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
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
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
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
	vkMapMemory(device, graphicsPipeline.uniformBuffers[index].bufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
	memcpy(data, &ubo, sizeof(UniformBufferObject));
	vkUnmapMemory(device, graphicsPipeline.uniformBuffers[index].bufferMemory);
}

void ModeledObject::CreateCommandBuffers()
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

void ModeledObject::CreateDescriptorSets(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	std::vector<VkImageView> imageViews;
	std::vector<VkSampler> samplers;
	size_t swapChainSize = swapChain.swapChainImages.size();
	uint32_t descriptorCount = static_cast<uint32_t>(swapChainSize);

#pragma region DESCRIPTOR_POOL
	VkDescriptorPoolSize poolSizes[2] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = descriptorCount;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = descriptorCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = descriptorCount;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	graphicsPipeline.isDescriptorPoolEmpty = false;
#pragma endregion

#pragma region BINDINGS
	// gather texture image views and samplers
	for (Material mat : object->materials)
	{
		if (mat.diffuseTex.has_value())
		{
			imageViews.push_back(mat.diffuseTex->imageView);
			samplers.push_back(mat.diffuseTex->sampler);
		}

		if (mat.normalTex.has_value())
		{
			imageViews.push_back(mat.normalTex->imageView);
			samplers.push_back(mat.normalTex->sampler);
		}

		if (mat.alphaTex.has_value())
		{
			imageViews.push_back(mat.alphaTex->imageView);
			samplers.push_back(mat.alphaTex->sampler);
		}

		if (mat.metallicTex.has_value())
		{
			imageViews.push_back(mat.metallicTex->imageView);
			samplers.push_back(mat.metallicTex->sampler);
		}

		if (mat.displacementTex.has_value())
		{
			imageViews.push_back(mat.displacementTex->imageView);
			samplers.push_back(mat.displacementTex->sampler);
		}

		if (mat.emissiveTex.has_value())
		{
			imageViews.push_back(mat.emissiveTex->imageView);
			samplers.push_back(mat.emissiveTex->sampler);
		}
	}

	size_t numSamplers = samplers.size();
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	// create binding of UBO
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;
	bindings.push_back(uboBinding);

	// create bindings for samplers
	for (uint32_t i = 0; i < numSamplers; i++)
	{
		// create descriptor set for texture
		VkDescriptorSetLayoutBinding samplerBinding{};
		samplerBinding.binding = i + 1;
		samplerBinding.descriptorCount = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(samplerBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
#pragma endregion


	// create uniform buffers for graphics pipeline
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	graphicsPipeline.uniformBuffers.resize(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++)
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			graphicsPipeline.uniformBuffers[i].buffer, graphicsPipeline.uniformBuffers[i].bufferMemory);
	
#pragma region POOLS_SET_LAYOUT
	

	std::vector<VkDescriptorSetLayout> layout(swapChainSize, graphicsPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = descriptorCount;
	allocInfo.pSetLayouts = layout.data();
	graphicsPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

#pragma endregion

#pragma region WRITES
	std::vector<VkWriteDescriptorSet> descriptorWrites(numSamplers + 1);

	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = graphicsPipeline.uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = graphicsPipeline.descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		for (uint32_t j = 0; j < numSamplers; j++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = imageViews[j];
			imageInfo.sampler = samplers[j];

			descriptorWrites[j + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[j + 1].dstSet = graphicsPipeline.descriptorSets[i];
			descriptorWrites[j + 1].dstBinding = j + 1;
			descriptorWrites[j + 1].dstArrayElement = 0;
			descriptorWrites[j + 1].descriptorCount = 1;
			descriptorWrites[j + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[j + 1].pImageInfo = &imageInfo; // used for descriptors that refer to image data
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
#pragma endregion
	
}
