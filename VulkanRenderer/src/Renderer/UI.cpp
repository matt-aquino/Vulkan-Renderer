#include "UI.h"
#include "Renderer.h"

UI::UI(const VkCommandPool& commandPool, const VulkanSwapChain& swapChain, const VulkanGraphicsPipeline& graphicsPipeline, VkSampleCountFlagBits counts)
	: commandPool(commandPool), graphicsPipeline(graphicsPipeline)
{
	VulkanDevice* vkDevice = VulkanDevice::GetVulkanDevice();
	device = vkDevice->GetLogicalDevice();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
	io.Fonts->AddFontDefault();
	ImGui::StyleColorsDark();

	VkExtent2D dim = swapChain.swapChainDimensions;
	io.DisplaySize = ImVec2(dim.width, dim.height);

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	ImGui_ImplSDL2_InitForVulkan(Renderer::GetWindow());

	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
	};
	uint32_t poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.poolSizeCount = poolSizeCount;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = poolSizeCount * 1000;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create UI descriptor pool");

	ImGui_ImplVulkan_InitInfo initInfo =
	{
		Renderer::GetVKInstance(),
		vkDevice->GetPhysicalDevice(),
		device,
		vkDevice->GetFamilyIndices().graphicsFamily.value(),
		vkDevice->GetQueues().renderQueue,
		nullptr,
		descriptorPool,
		0,
		2,
		swapChain.swapChainImages.size(),
		counts,
		nullptr, nullptr
	};

	ImGui_ImplVulkan_Init(&initInfo, graphicsPipeline.renderPass);
	VkCommandBuffer fontCmdBuffer = HelperFunctions::beginSingleTimeCommands(commandPool);
	ImGui_ImplVulkan_CreateFontsTexture(fontCmdBuffer);
	HelperFunctions::endSingleTimeCommands(fontCmdBuffer, vkDevice->GetQueues().renderQueue, commandPool);


	//CreateFontTexture(vkDevice->GetQueues().renderQueue);
	//CreateRenderPass(swapChain.swapChainImageFormat);
	//CreateDescriptors(swapChain.swapChainImages.size());
	//CreateFramebuffers(swapChain);
	//CreateGraphicsPipeline();

	/*
	ImGui_ImplSDL2_InitForVulkan(Renderer::GetWindow());
	ImGui_ImplVulkan_InitInfo initInfo =
	{
		Renderer::GetVKInstance(),
		vkDevice->GetPhysicalDevice(),
		device,
		vkDevice->GetFamilyIndices().graphicsFamily.value(),
		vkDevice->GetQueues().renderQueue,
		nullptr,
		graphicsPipeline.descriptorPool,
		0,
		2,
		swapChain.swapChainImages.size(),
		VK_SAMPLE_COUNT_1_BIT,
		nullptr, nullptr
	};

	ImGui_ImplVulkan_Init(&initInfo, graphicsPipeline.renderPass);
	*/

	
}

UI::~UI()
{
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void UI::NewUIFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void UI::EndFrame()
{
	ImGui::Render();

	
	//UpdateBuffers();
}

void UI::RenderFrame(VkCommandBuffer commandBuffer, uint32_t index)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	return;

	VkClearValue clearColors = {};
	clearColors.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.framebuffer = graphicsPipeline.framebuffers[index];
	renderPassInfo.renderPass = graphicsPipeline.renderPass;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColors;
	renderPassInfo.renderArea = { int(io.DisplaySize.x), int(io.DisplaySize.y) };

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 0, 1, &graphicsPipeline.descriptorSets[index], 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);

	VkViewport viewport = {};
	viewport.width = io.DisplaySize.x;
	viewport.height = io.DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	// UI scale and translate via push constants
	pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(commandBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstBlock), &pushConstBlock);

	// Render commands
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;


	if (draw_data->CmdListsCount > 0)
	{
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &graphicsPipeline.vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, graphicsPipeline.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[i];

			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];

				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}

	vkCmdEndRenderPass(commandBuffer);
}

bool UI::NewWindow(const char* name)
{
	return ImGui::Begin(name);
}

void UI::EndWindow()
{
	ImGui::End();
}

void UI::AddSpacing(int numSpaces)
{
	for (int i = 0; i < numSpaces; i++)
		ImGui::NewLine();
}


void UI::DrawUIText(const char* text)
{
	ImGui::Text(text);
}

void UI::DrawUITextFloat(const char* name, float& value)
{
	const char* text = "%s: %.2f";
	ImGui::Text(text, name, value);
}

void UI::DrawUITextVec2(const char* name, glm::vec2& values)
{
	const char* text = "%s: %.2f, %.2f";
	ImGui::Text(text, name, values[0], values[1]);
}

void UI::DrawUITextVec3(const char* name, glm::vec3& values)
{
	const char* text = "%s: %.2f, %.2f, %.2f";
	ImGui::Text(text, name, values[0], values[1], values[2]);
}

void UI::DrawUITextVec4(const char* name, glm::vec4& values)
{
	const char* text = "%s: %.2f, %.2f, %.2f, %.2f";
	ImGui::Text(text, name, values[0], values[1], values[2], values[3]);
}

bool UI::DrawCheckBox(const char* label, bool* value)
{
	return ImGui::Checkbox(label, value);
}

ImTextureID UI::AddImage(VkSampler& sampler, VkImageView& imageView, VkImageLayout layout)
{
	if (addedTextures.find(imageView) != addedTextures.end())
		return addedTextures[imageView];

	ImTextureID texID = ImGui_ImplVulkan_AddTexture(sampler, imageView, layout);
	addedTextures.insert({ imageView, texID });
	return texID;
}

void UI::DrawImage(const char* name, Texture texture, VkSampler* sampler, glm::vec2 size, VkImageLayout layout)
{
	ImGui::Text(name);
	ImTextureID texID = AddImage(*sampler, texture.imageView, layout);
	ImVec2 imageSize = ImVec2(size.x, size.y);
	ImGui::Image(texID, imageSize);
}

void UI::DrawImage(const char* name, VkSampler* sampler, VkImageView* imageView, glm::vec2 size, VkImageLayout layout)
{
	ImGui::Text(name);
	ImTextureID texID = AddImage(*sampler, *imageView, layout);
	ImVec2 imageSize = ImVec2(size.x, size.y);
	ImGui::Image(texID, imageSize);
}

void UI::ShowDemoWindow()
{
	ImGui::ShowDemoWindow();
}

void UI::DisplayFPS()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS: %2f", io.Framerate);
}

bool UI::DrawSliderFloat(const char* name, float* value, float minValue, float maxValue)
{
	return ImGui::SliderFloat(name, value, minValue, maxValue);
}

bool UI::DrawSliderVec2(const char* name, glm::vec2* values, float minValue, float maxValue)
{
	return ImGui::SliderFloat2(name, &values->x, minValue, maxValue);
}

bool UI::DrawSliderVec3(const char* name, glm::vec3* values, float minValue, float maxValue)
{
	return ImGui::SliderFloat3(name, &values->x, minValue, maxValue);
}

bool UI::DrawSliderVec4(const char* name, glm::vec4* values, float minValue, float maxValue)
{
	return ImGui::SliderFloat4(name, &values->x, minValue, maxValue);
}

bool UI::NewTreeNode(const char* label)
{
	return ImGui::TreeNode(label);
}

bool UI::NewTreeNode(const void* ptr, const char* label, ...)
{
	va_list args;
	va_start(args, label);
	bool is_open = ImGui::TreeNodeExV(ptr, 0, label, args);
	va_end(args);
	return is_open;
}

void UI::EndTreeNode()
{
	ImGui::TreePop();
}

void UI::SetMouseCapture(bool capture)
{
	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureMouse = capture;
}

void UI::SetKeyboardCapture(bool capture)
{
	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureKeyboard = capture;
}

void UI::SetTextCapture(bool capture)
{
	ImGuiIO& io = ImGui::GetIO();
	io.WantTextInput = capture;
}

void UI::AddMouseButtonEvent(int button, bool pressed)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, pressed);
}

void UI::AddMouseWheelEvent(float mouseWheelX, float mouseWheelY)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseWheelEvent(mouseWheelX, mouseWheelY);
}

void UI::AddKeyEvent(int key, bool pressed)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddKeyEvent((ImGuiKey)key, pressed);
}

void UI::UpdateBuffers()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}

	// Update buffers only if vertex or index count has been changed compared to current buffer size

	// Vertex buffer
	if ((graphicsPipeline.vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) 
	{
		graphicsPipeline.vertexBuffer.destroy();

		HelperFunctions::createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			graphicsPipeline.vertexBuffer.buffer, graphicsPipeline.vertexBuffer.bufferMemory);
		
		vertexCount = imDrawData->TotalVtxCount;
		graphicsPipeline.vertexBuffer.map();
	}

	// Index buffer
	if ((graphicsPipeline.indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) 
	{
		graphicsPipeline.indexBuffer.destroy();

		HelperFunctions::createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			graphicsPipeline.indexBuffer.buffer, graphicsPipeline.indexBuffer.bufferMemory);
		
		indexCount = imDrawData->TotalIdxCount;
		graphicsPipeline.indexBuffer.map();
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)graphicsPipeline.vertexBuffer.mappedMemory;
	ImDrawIdx* idxDst = (ImDrawIdx*)graphicsPipeline.indexBuffer.mappedMemory;

	for (int n = 0; n < imDrawData->CmdListsCount; n++) 
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	graphicsPipeline.vertexBuffer.flush();
	graphicsPipeline.indexBuffer.flush();
}



void UI::CreateRenderPass(VkFormat swapChainFormat)
{
	VkAttachmentDescription attachment = {};
	attachment.format = swapChainFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // load contents from scene pass
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // we don't really need to store, but we will in case
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	// UI pass must wait for scene render pass to finish before drawing on top
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	if (vkCreateRenderPass(device, &info, nullptr, &graphicsPipeline.renderPass) != VK_SUCCESS)
		throw std::runtime_error("Could not create Dear ImGui's render pass");
	
}

void UI::CreateDescriptors(int numImages)
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.descriptorCount = numImages;
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = numImages;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create UI descriptor pool");
	graphicsPipeline.isDescriptorPoolEmpty = false;

	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create UI descriptor set layout");

	std::vector<VkDescriptorSetLayout> layout(numImages, graphicsPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = numImages;
	allocInfo.pSetLayouts = layout.data();
	graphicsPipeline.descriptorSets.resize(numImages);

	if (vkAllocateDescriptorSets(device, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate UI descriptor sets");

	for (size_t i = 0; i < numImages; i++)
	{
		VkWriteDescriptorSet write = HelperFunctions::initializers::writeDescriptorSet(graphicsPipeline.descriptorSets[i], &fontTexture.textureDescriptor);
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}
}

void UI::CreateFontTexture(VkQueue renderQueue)
{
	//VkCommandBuffer fontCmdBuffer = HelperFunctions::beginSingleTimeCommands(commandPool);
	//ImGui_ImplVulkan_CreateFontsTexture(fontCmdBuffer);
	//HelperFunctions::endSingleTimeCommands(fontCmdBuffer, renderQueue, commandPool);

	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	size_t upload_size = width * height * 4 * sizeof(char);

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

	HelperFunctions::createImage(width, height, 1, 1, VK_SAMPLE_COUNT_1_BIT,
		imageType, format, tiling,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		fontTexture.image, fontTexture.imageMemory);

	HelperFunctions::createImageView(fontTexture.image, fontTexture.imageView,
		format, VK_IMAGE_ASPECT_COLOR_BIT, imageViewType, 1);

	VulkanBuffer stagingBuffer = {};
	HelperFunctions::createBuffer(upload_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer.buffer, stagingBuffer.bufferMemory);

	stagingBuffer.map();
	memcpy(stagingBuffer.mappedMemory, pixels, upload_size);
	stagingBuffer.unmap();

	HelperFunctions::transitionImageLayout(fontTexture.image, format, 1,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		commandPool, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	HelperFunctions::copyBufferToImage(commandPool,
		stagingBuffer.buffer, fontTexture.image,
		width, height, 1);

	HelperFunctions::transitionImageLayout(fontTexture.image, format, 1,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		commandPool, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	HelperFunctions::createSampler(fontTextureSampler, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	fontTexture.textureDescriptor.imageView = fontTexture.imageView;
	fontTexture.textureDescriptor.sampler = fontTextureSampler;
	fontTexture.textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	io.Fonts->SetTexID((ImTextureID)&fontTexture);
	stagingBuffer.destroy();

}

void UI::CreateFramebuffers(const VulkanSwapChain& swapChain)
{
	size_t numImages = swapChain.swapChainImages.size();
	VkExtent2D swapChainDim = swapChain.swapChainDimensions;
	graphicsPipeline.framebuffers.resize(numImages);

	VkImageView attachment = {};

	for (int i = 0; i < numImages; i++)
	{
		attachment = swapChain.swapChainImageViews[i];
		VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = &attachment;
		fbInfo.renderPass = graphicsPipeline.renderPass;
		fbInfo.width = swapChainDim.width;
		fbInfo.height = swapChainDim.height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &graphicsPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}
	
}

void UI::CreateGraphicsPipeline()
{
	auto vertCode = HelperFunctions::readShaderFile("shaders/ImGui/imgui_vert.spv");
	auto fragCode = HelperFunctions::readShaderFile("shaders/ImGui/imgui_frag.spv");

	VkShaderModule vertModule = HelperFunctions::CreateShaderModules(vertCode);
	VkShaderModule fragModule = HelperFunctions::CreateShaderModules(fragCode);

	VkPipelineShaderStageCreateInfo stages[] =
	{
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule),
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule)
	};

	VkVertexInputBindingDescription bindingDesc = FontVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attrDesc = FontVertex::getAttributeDescriptions();

	VkPipelineColorBlendAttachmentState blendAttachState = {};
	blendAttachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachState.blendEnable = VK_FALSE;
	blendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = HelperFunctions::initializers::pipelineInputAssemblyStateCreateInfo();
	VkPipelineVertexInputStateCreateInfo vertexInputState = HelperFunctions::initializers::pipelineVertexInputStateCreateInfo(1, bindingDesc, 3, attrDesc.data());
	VkPipelineColorBlendStateCreateInfo colorBlendState = HelperFunctions::initializers::pipelineColorBlendStateCreateInfo(1, blendAttachState);
	VkPipelineMultisampleStateCreateInfo multisampleState = HelperFunctions::initializers::pipelineMultisampleStateCreateInfo();
	VkPipelineRasterizationStateCreateInfo rasterizerState = HelperFunctions::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);
	VkPipelineViewportStateCreateInfo viewportState = HelperFunctions::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = HelperFunctions::initializers::pipelineDynamicStateCreateInfo(2, dynamicStates);

	VkPushConstantRange pushRange = {};
	pushRange.offset = 0;
	pushRange.size = sizeof(pushConstBlock);
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushRange;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &graphicsPipeline.descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create UI pipeline layout");

	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.layout = graphicsPipeline.pipelineLayout;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pRasterizationState = &rasterizerState;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.renderPass = graphicsPipeline.renderPass;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;

	if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline.pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create UI graphics pipeline");

	vkDestroyShaderModule(device, vertModule, nullptr);
	vkDestroyShaderModule(device, fragModule, nullptr);
}