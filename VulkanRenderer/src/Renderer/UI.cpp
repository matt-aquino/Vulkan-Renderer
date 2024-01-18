#include "UI.h"
#include "Renderer.h"

UI::UI(const VkCommandPool& commandPool, const VulkanSwapChain& swapChain, const VkRenderPass& renderPass, 
	const VulkanGraphicsPipeline& graphicsPipeline, VkSampleCountFlagBits counts)
	: commandPool(commandPool), renderPass(renderPass), graphicsPipeline(graphicsPipeline)
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
	io.DisplaySize = ImVec2((float)dim.width, (float)dim.height);

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
		static_cast<uint32_t>(swapChain.swapChainImages.size()),
		counts,
		nullptr, nullptr
	};

	ImGui_ImplVulkan_Init(&initInfo, renderPass);
	VkCommandBuffer fontCmdBuffer = HelperFunctions::beginSingleTimeCommands(commandPool);
	ImGui_ImplVulkan_CreateFontsTexture(fontCmdBuffer);
	HelperFunctions::endSingleTimeCommands(fontCmdBuffer, vkDevice->GetQueues().renderQueue, commandPool);
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

void UI::RenderFrame(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t index)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	return;

	VkClearValue clearColors = {};
	clearColors.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderPass = renderPass;
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
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

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