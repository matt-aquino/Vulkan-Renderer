#include "PBR.h"

PBR::PBR(const std::string name, const VulkanSwapChain& swapChain)
{
	sceneName = name;
	logicalDevice = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	swapChainImageFormat = swapChain.swapChainImageFormat;
	
	CreateSkyboxPipeline();
}

PBR::~PBR()
{
}

void PBR::RecreateScene(const VulkanSwapChain& swapChain)
{
}

void PBR::RecordScene()
{
}

VulkanReturnValues PBR::PresentScene(const VulkanSwapChain& swapChain)
{
	return VulkanReturnValues();
}

void PBR::DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
}

void PBR::DestroyScene(bool isRecreation)
{
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
	}
	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{
		uniformBuffers[i].destroy();
	}
	vkDestroyImageView(device, depthStencilBufferView, nullptr);
	vkDestroyImage(device, depthStencilBuffer, nullptr);
	vkFreeMemory(device, depthStencilBufferMemory, nullptr);
}

void PBR::HandleKeyboardInput(const uint8_t* keystates, float dt)
{
}

void PBR::HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY)
{
}

void PBR::CreateRenderPass()
{
	VkAttachmentDescription attachments[2] = {};

	// color
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].format = swapChainImageFormat;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;

	// depth
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].format = VK_FORMAT_D16_UNORM_S8_UINT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentReference references[2] = {};
	references[0].attachment = 0;
	references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	references[1].attachment = 1;
	references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// wait for other passes to finish rendering 
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

	VkSubpassDescription subpass = {};
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &references[0];
	subpass.pDepthStencilAttachment = &references[1];
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	VkRenderPassCreateInfo passInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	passInfo.attachmentCount = 2;
	passInfo.pAttachments = attachments;
	passInfo.dependencyCount = 2;
	passInfo.pDependencies = dependencies;
	passInfo.subpassCount = 1;
	passInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(logicalDevice, &passInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create skybox render pass");
}

void PBR::CreateSkyboxPipeline()
{
	skyboxCube = BasicShapes::createBox(100.0f, 100.0f, 100.0f);
	
	// load in texture
	skyboxTexture = TextureLoader::loadTexture("assets/textures/skybox", "alps_field.hdr", TextureType::NONE,
		VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	// descriptor sets and pipeline layout
	{
		VkDescriptorPoolSize poolSizes[] =
		{ 
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}
		};

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.maxSets = 6;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes;

		if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &skyboxPipeline.descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create skybox descriptor pool");

		VkDescriptorSetLayoutBinding bindings[2] = {};
		bindings[0].binding = 0;
		bindings[0].descriptorCount = 1;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorCount = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo setLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		setLayoutInfo.bindingCount = 2;
		setLayoutInfo.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(logicalDevice, &setLayoutInfo, nullptr, &skyboxPipeline.descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create skybox descriptor set layout");

		skyboxPipeline.descriptorSets.resize(1);
		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = skyboxPipeline.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &skyboxPipeline.descriptorSetLayout;

		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &skyboxPipeline.descriptorSets[0]) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate skybox descriptor sets");

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageView = skyboxTexture->imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.sampler = skyboxTexture->sampler;

		VkWriteDescriptorSet write = HelperFunctions::initializers::writeDescriptorSet(skyboxPipeline.descriptorSets[0], &imageInfo, 1);

		vkUpdateDescriptorSets(logicalDevice, 1, &write, 0, 0);


		VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &skyboxPipeline.descriptorSetLayout;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &skyboxPipeline.pipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create skybox pipeline layout");
	}

	// graphics pipeline
	{
		VkPipelineColorBlendAttachmentState attachmentState = {};
		attachmentState.blendEnable = VK_FALSE;
		attachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		attachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		attachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		VkPipelineColorBlendStateCreateInfo colorBlendState = HelperFunctions::initializers::pipelineColorBlendStateCreateInfo(1, attachmentState);

		std::array<VkVertexInputAttributeDescription, 3> attrDesc = ModelVertex::getAttributeDescriptions();
		VkVertexInputBindingDescription bindDesc = ModelVertex::getBindingDescription();
		VkPipelineVertexInputStateCreateInfo vertexState = HelperFunctions::initializers::pipelineVertexInputStateCreateInfo(1, bindDesc, 3, attrDesc.data());

		VkPipelineDepthStencilStateCreateInfo depthStencilState = HelperFunctions::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = HelperFunctions::initializers::pipelineInputAssemblyStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo multisampleState = HelperFunctions::initializers::pipelineMultisampleStateCreateInfo();
		VkPipelineRasterizationStateCreateInfo rasterizerState = HelperFunctions::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT);
		VkPipelineViewportStateCreateInfo viewportState = HelperFunctions::initializers::pipelineViewportStateCreateInfo(1, 1, 0);


		auto vertShaderCode = HelperFunctions::readShaderFile("Global/skybox_vert.spv");
		auto fragShaderCode = HelperFunctions::readShaderFile("Global/skybox_frag.spv");

		VkShaderModule vertModule = HelperFunctions::CreateShaderModules(vertShaderCode);
		VkShaderModule fragModule = HelperFunctions::CreateShaderModules(fragShaderCode);

		VkPipelineShaderStageCreateInfo shaderStages[2] =
		{
			HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule),
			HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule),
		};

		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.basePipelineHandle = nullptr;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.layout = skyboxPipeline.pipelineLayout;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pVertexInputState = &vertexState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(logicalDevice, nullptr, 1, &pipelineInfo, nullptr, &skyboxPipeline.pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create skybox pipeline");

		vkDestroyShaderModule(logicalDevice, vertModule, nullptr);
		vkDestroyShaderModule(logicalDevice, fragModule, nullptr);
	}
	

}

void PBR::CreateScenePipeline()
{
}

void PBR::CreatePostProcessPipeline()
{
}

void PBR::CreateFramebuffers()
{
}

void PBR::CreateCommandBuffers()
{
}

void PBR::CreateSyncObjects()
{
	// create semaphores
	renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create sync objects for a frame");
	}
}
