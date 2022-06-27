#include "DeferredRendering.h"


// SCENE SETUP
DeferredRendering::DeferredRendering(const std::string sceneName, const VulkanSwapChain& swapChain)
{
	this->sceneName = sceneName;
	device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	srand(unsigned int(time(NULL)));
	CreateSyncObjects();

	// create offscreen pipeline
	CreateOffscreenPipelineResources(swapChain);
	CreateOffscreenRenderPass(swapChain);
	CreateOffscreenFramebuffers(swapChain);
	CreateOffscreenPipeline(swapChain);

	
	// create composition pipeline
	CreateCompositionPipelineResources(swapChain);
	CreateCompositionRenderPass(swapChain);
	CreateCompositionFramebuffers(swapChain);
	CreateCompositionPipeline(swapChain);

	CreateCommandBuffers();
	CreateSceneObjects(swapChain);
	ui = new UI(commandPool, swapChain, compositionPipeline, VK_SAMPLE_COUNT_1_BIT);

}

DeferredRendering::~DeferredRendering()
{
	DestroyScene(false);
	delete ui;
}


void DeferredRendering::RecreateScene(const VulkanSwapChain& swapChain)
{
	DestroyScene(true);
	delete ui;

	// create offscreen pipeline
	CreateOffscreenPipelineResources(swapChain);
	CreateOffscreenRenderPass(swapChain);
	CreateOffscreenFramebuffers(swapChain);
	CreateOffscreenPipeline(swapChain);

	// create composition pipeline
	CreateCompositionPipelineResources(swapChain);
	CreateCompositionRenderPass(swapChain);
	CreateCompositionFramebuffers(swapChain);
	CreateCompositionPipeline(swapChain);

	CreateCommandBuffers();
	ui = new UI(commandPool, swapChain, compositionPipeline, VK_SAMPLE_COUNT_1_BIT);
}

void DeferredRendering::DestroyScene(bool isRecreation)
{
	offscreenPipeline.destroyGraphicsPipeline(device);
	compositionPipeline.destroyGraphicsPipeline(device);

	colorTexture.destroyTexture();
	normalTexture.destroyTexture();
	positionTexture.destroyTexture();
	vkDestroySampler(device, textureSampler, nullptr);

	uint32_t size = static_cast<uint32_t>(commandBuffersList.size());
	vkFreeCommandBuffers(device, commandPool, size, commandBuffersList.data());

	if (!isRecreation)
	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		for (int i = 0; i < 100; i++)
			spheres[i].destroyMesh();

		plane.destroyMesh();
	}
}

void DeferredRendering::CreateSceneObjects(const VulkanSwapChain& swapChain)
{
	sceneCamera = Camera::GetCamera();
	VkExtent2D dim = swapChain.swapChainDimensions;
	proj = glm::perspective(glm::radians(45.0f), float(dim.width) / float(dim.height), 0.1f, 1000.0f);
	proj[1][1] *= -1;
	deferredUBO.viewProj = proj * sceneCamera->GetViewMatrix();

	offscreenPipeline.uniformBuffers[0].map();
	memcpy(offscreenPipeline.uniformBuffers[0].mappedMemory, &deferredUBO, sizeof(deferredUBO));
	offscreenPipeline.uniformBuffers[0].unmap();

	glm::mat4 scale = glm::scale(glm::vec3(0.25f));
	glm::mat4 model = glm::mat4(1.0f);

	// 100 lights, positioned at -5 through 4 on x and z axes, so 10 each row
	int row = -5, col = -5;
	for (int i = 0; i < 100; i++)
	{
		if (col > 0 && col % 5 == 0)
		{
			col = -5;
			row++;
		}
		glm::vec4 position = glm::vec4(col, 1, row, 1);

		//float r = glm::clamp(float((rand() % 255 + 128) / 255.0f), 0.0f, 1.0f);
		//float g = glm::clamp(float((rand() % 255 + 128) / 255.0f), 0.0f, 1.0f);
		//float b = glm::clamp(float((rand() % 255 + 128) / 255.0f), 0.0f, 1.0f);
		//glm::vec4 color = glm::vec4(r, g, b, 1.0f);
		glm::vec4 color = glm::vec4(1.0, 0.0, 0.0, 1.0);
		float radius = 1.0f;
		float intensity = 1.0f;

		compositionUBO.lights[i] = { position, color, radius, intensity };

		position.y = -0.75f;
		model = glm::translate(glm::vec3(position)) * scale;
		spheres[i] = BasicShapes::createSphere();
		spheres[i].setModelMatrix(model);
		spheres[i].setMaterialWithPreset(MaterialPresets::WHITE_PLASTIC);

		col++;
	}

	for (int i = 0; i < 3; i++)
	{
		compositionPipeline.uniformBuffers[i].map();
		memcpy(compositionPipeline.uniformBuffers[i].mappedMemory, &compositionUBO, sizeof(compositionUBO));
		compositionPipeline.uniformBuffers[i].unmap();
	}
	
	plane = BasicShapes::createPlane();
	plane.setMaterialWithPreset(MaterialPresets::BLACK_PLASTIC);
	model = glm::translate(glm::vec3(-0.5f, -1.0f, 0.0f)) * glm::scale(glm::vec3(10.0f));
	plane.setModelMatrix(model);
}

void DeferredRendering::CreateSyncObjects()
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
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create sync objects for a frame");
	}
}

void DeferredRendering::CreateCommandBuffers()
{
	commandBuffersList.resize(compositionPipeline.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffersList.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffersList.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");
}

// DRAWING AND PRESENTATION

// for when we want to prebake command buffers
void DeferredRendering::RecordScene()
{
}

VulkanReturnValues DeferredRendering::PresentScene(const VulkanSwapChain& swapChain)
{

	static auto queues = VulkanDevice::GetVulkanDevice()->GetQueues();

	uint32_t imageIndex;

	/*
	********** PREPARATION ************
	*/
	compositionPipeline.result = vkAcquireNextImageKHR(device, swapChain.swapChain,
		UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (compositionPipeline.result == VK_ERROR_OUT_OF_DATE_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (compositionPipeline.result != VK_SUCCESS && compositionPipeline.result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire swap chain image");

	// check if a previous frame is using this image
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

	Update(imageIndex); // update matrices as needed
	RecordCommandBuffer(imageIndex);

	// mark image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

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

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(queues.renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer!");

	/*
	********* PRESENTATION *************
	*/
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	compositionPipeline.result = vkQueuePresentKHR(queues.presentQueue, &presentInfo);

	if (compositionPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || compositionPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (compositionPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;


	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void DeferredRendering::DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
	plane.draw(commandBuffer, pipelineLayout, useMaterial);

	for (int i = 0; i < 100; i++)
	{
		spheres[i].draw(commandBuffer, pipelineLayout, useMaterial);
	}
}


void DeferredRendering::Update(uint32_t index)
{
	deferredUBO.viewProj = proj * sceneCamera->GetViewMatrix();

	offscreenPipeline.uniformBuffers[0].map();
	memcpy(offscreenPipeline.uniformBuffers[0].mappedMemory, &deferredUBO, sizeof(deferredUBO));
	offscreenPipeline.uniformBuffers[0].unmap();	
}

// record a command buffer every frame
void DeferredRendering::RecordCommandBuffer(uint32_t index)
{

	VkCommandBufferBeginInfo cmdBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkRenderPassBeginInfo rpBI = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

	VkClearValue clearValues[4];
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkDeviceSize offsets[] = { 0 };

	if (vkBeginCommandBuffer(commandBuffersList[index], &cmdBI) != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer");

	// render G-Buffer textures offscreen
	{
		rpBI.renderPass = offscreenPipeline.renderPass;
		rpBI.framebuffer = offscreenPipeline.framebuffers[index];
		rpBI.renderArea.extent = offscreenPipeline.scissors.extent;
		rpBI.clearValueCount = 4;
		rpBI.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffersList[index], &rpBI, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(commandBuffersList[index], 0, 1, &offscreenPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &offscreenPipeline.scissors);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline.pipelineLayout,
			0, 1, &offscreenPipeline.descriptorSets[0], 0, nullptr);

		

		DrawScene(commandBuffersList[index], offscreenPipeline.pipelineLayout, true);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	// compose the final scene
	{
		rpBI.renderPass = compositionPipeline.renderPass;
		rpBI.framebuffer = compositionPipeline.framebuffers[index];
		rpBI.renderArea.extent = compositionPipeline.scissors.extent;
		rpBI.clearValueCount = 1;
		rpBI.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffersList[index], &rpBI, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(commandBuffersList[index], 0, 1, &compositionPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &compositionPipeline.scissors);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, compositionPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, compositionPipeline.pipelineLayout,
			0, 1, &compositionPipeline.descriptorSets[index], 0, nullptr);


		vkCmdDraw(commandBuffersList[index], 3, 1, 0, 0);

		DrawUI(index);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	if (vkEndCommandBuffer(commandBuffersList[index]) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer");

}

void DeferredRendering::DrawUI(uint32_t index)
{
	ui->NewUIFrame();
	{
		ui->NewWindow("Application");
		{
			ui->DisplayFPS();
			glm::vec3 camPos = sceneCamera->GetCameraPosition();
			ui->DrawUITextVec3("Camera Position", camPos);
		}
		ui->EndWindow();

		static int textureChoice = 0;
		ui->NewWindow("G Buffer Textures");
		{
			if (ImGui::BeginMenu("Texture"))
			{
				if (ImGui::MenuItem("Colors"))
					textureChoice = 0;

				else if (ImGui::MenuItem("Normals"))
					textureChoice = 1;
				
				else if (ImGui::MenuItem("Positions"))
					textureChoice = 2;

				ImGui::EndMenu();
			}

			switch (textureChoice)
			{
				case 0:
					ui->DrawImage("Colors", colorTexture, &colorTexture.sampler, glm::vec2(320, 180)); // 1280x720 divided by 4 to keep aspect ratio
					break;

				case 1:
					ui->DrawImage("Normals", normalTexture, &normalTexture.sampler, glm::vec2(320, 180));
					break;

				case 2:
					ui->DrawImage("Positions", positionTexture, &positionTexture.sampler, glm::vec2(320, 180));
					break;

				default:
					break;
			}
		}
		ui->EndWindow();
	}
	ui->EndFrame();
	
	ui->RenderFrame(commandBuffersList[index], index);
}

// INPUT

void DeferredRendering::HandleKeyboardInput(const uint8_t* keystates, float dt)
{
	static Camera* camera = Camera::GetCamera();

	if (isCameraMoving)
	{
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
}

void DeferredRendering::HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY)
{
	static Camera* camera = Camera::GetCamera();

	static float deltaX = 0.0f;
	static float deltaY = 0.0f;

	isCameraMoving = ((buttons & SDL_BUTTON_RMASK) != 0);

	if (isCameraMoving)
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);
		ui->SetMouseCapture(false);

		float sensivity = 0.1f;
		deltaX = x * sensivity;
		deltaY = y * sensivity;

		camera->RotateCamera(deltaX, deltaY);
	}

	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
		ui->SetMouseCapture(true);
		ui->AddMouseButtonEvent(0, (buttons & SDL_BUTTON_LMASK) != 0);
		ui->AddMouseWheelEvent(mouseWheelX, mouseWheelY);
	}
}


// OFFSCREEN PIPELINE GENERATION

void DeferredRendering::CreateOffscreenPipelineResources(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;
	
	// g-buffer textures
	{
		// color
		HelperFunctions::createImage(dim.width, dim.height, 1, 1,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			colorTexture.image, colorTexture.imageMemory);

		HelperFunctions::createImageView(colorTexture.image, colorTexture.imageView,
			VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

		HelperFunctions::createSampler(colorTexture.sampler, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		// depth
		HelperFunctions::createImage(dim.width, dim.height, 1, 1,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_D16_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			offscreenPipeline.depthStencilBuffer, offscreenPipeline.depthStencilBufferMemory);

		HelperFunctions::createImageView(offscreenPipeline.depthStencilBuffer, offscreenPipeline.depthStencilBufferView,
			VK_FORMAT_D16_UNORM, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

		HelperFunctions::createSampler(textureSampler, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		offscreenPipeline.isDepthBufferEmpty = false;

		// normals
		HelperFunctions::createImage(dim.width, dim.height, 1, 1,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			normalTexture.image, normalTexture.imageMemory);

		HelperFunctions::createImageView(normalTexture.image, normalTexture.imageView,
			VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

		HelperFunctions::createSampler(normalTexture.sampler, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		// positions
		HelperFunctions::createImage(dim.width, dim.height, 1, 1,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			positionTexture.image, positionTexture.imageMemory);

		HelperFunctions::createImageView(positionTexture.image, positionTexture.imageView,
			VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

		HelperFunctions::createSampler(positionTexture.sampler, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	}

	// uniform buffer
	{
		offscreenPipeline.uniformBuffers.resize(1);
		HelperFunctions::createBuffer(sizeof(deferredUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			offscreenPipeline.uniformBuffers[0].buffer, offscreenPipeline.uniformBuffers[0].bufferMemory);
	}

	// descriptors
	{
		VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };

		VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;
		poolCreateInfo.maxSets = 3;
		if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &offscreenPipeline.descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create offscreen pipeline descriptor pool");
		offscreenPipeline.isDescriptorPoolEmpty = false;

		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = &binding;
		vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &offscreenPipeline.descriptorSetLayout);


		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = offscreenPipeline.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &offscreenPipeline.descriptorSetLayout;
		offscreenPipeline.descriptorSets.resize(1);

		if (vkAllocateDescriptorSets(device, &allocInfo, &offscreenPipeline.descriptorSets[0]) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor set");

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = offscreenPipeline.uniformBuffers[0].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(deferredUBO);
		VkWriteDescriptorSet write = HelperFunctions::initializers::writeDescriptorSet(offscreenPipeline.descriptorSets[0], &bufferInfo);
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}
}

void DeferredRendering::CreateOffscreenRenderPass(const VulkanSwapChain& swapChain)
{
	VkAttachmentDescription colorDesc = {}, depthDesc = {}, normalDesc = {}, posDesc = {};
	// color
	colorDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	colorDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// depth
	depthDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthDesc.format = VK_FORMAT_D16_UNORM;
	depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// normals
	normalDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	normalDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	normalDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// positions
	posDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	posDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	posDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	posDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	posDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	posDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	posDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	posDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference colorRef = {}, depthRef = {}, normalRef = {}, posRef = {};

	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	normalRef.attachment = 2;
	normalRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	posRef.attachment = 3;
	posRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	std::array<VkAttachmentReference, 3> colorAttachmentReferences;
	colorAttachmentReferences[0] = colorRef;
	colorAttachmentReferences[1] = normalRef;
	colorAttachmentReferences[2] = posRef;

	std::array<VkAttachmentDescription, 4> attachments;
	attachments[0] = colorDesc;
	attachments[1] = depthDesc;
	attachments[2] = normalDesc;
	attachments[3] = posDesc;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.colorAttachmentCount = 3;
	subpassDesc.pColorAttachments = colorAttachmentReferences.data();
	subpassDesc.pDepthStencilAttachment = &depthRef;
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	std::array<VkSubpassDependency,2> dependencies;
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
	// mine
	/*
	// wait for composition pipeline to finish reading from g-buffer textures
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// after rendering g-buffer, transition images to readable layout
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	*/
	

	VkRenderPassCreateInfo rpCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	rpCreateInfo.attachmentCount = 4;
	rpCreateInfo.pAttachments = attachments.data();
	rpCreateInfo.dependencyCount = 2;
	rpCreateInfo.pDependencies = dependencies.data();
	rpCreateInfo.subpassCount = 1;
	rpCreateInfo.pSubpasses = &subpassDesc;

	if (vkCreateRenderPass(device, &rpCreateInfo, nullptr, &offscreenPipeline.renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create offscreen render pass");
}

void DeferredRendering::CreateOffscreenPipeline(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;

	VkVertexInputBindingDescription bindings = ModelVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributes = ModelVertex::getAttributeDescriptions();
	
	VkPipelineColorBlendAttachmentState attachmentStates[3] = {};
	{
		attachmentStates[0].blendEnable = VK_TRUE;
		attachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		attachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
		attachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
		attachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		attachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		attachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		attachmentStates[1] = attachmentStates[0];
		attachmentStates[2] = attachmentStates[0];
	}

	VkDynamicState states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	auto dynamicState = HelperFunctions::initializers::pipelineDynamicStateCreateInfo(2, states);
	auto inputAssemblyState = HelperFunctions::initializers::pipelineInputAssemblyStateCreateInfo();
	auto vertexInputState = HelperFunctions::initializers::pipelineVertexInputStateCreateInfo(1, bindings, 3, attributes.data());
	auto colorBlendState = HelperFunctions::initializers::pipelineColorBlendStateCreateInfo(3, *attachmentStates);
	auto multisampleState = HelperFunctions::initializers::pipelineMultisampleStateCreateInfo();
	auto depthStencilState = HelperFunctions::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto rasterizerState = HelperFunctions::initializers::pipelineRasterizationStateCreateInfo();
	auto viewportState = HelperFunctions::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

	offscreenPipeline.viewport.x = 0.0f;
	offscreenPipeline.viewport.y = 0.0f;
	offscreenPipeline.viewport.minDepth = 0.0f;
	offscreenPipeline.viewport.maxDepth = 1.0f;
	offscreenPipeline.viewport.width = (float)dim.width;
	offscreenPipeline.viewport.height = (float)dim.height;
	offscreenPipeline.scissors.offset = { 0, 0 };
	offscreenPipeline.scissors.extent = dim;
	viewportState.pViewports = &offscreenPipeline.viewport;
	viewportState.pScissors = &offscreenPipeline.scissors;

	auto vertCode = HelperFunctions::readShaderFile(SHADERPATH"DeferredRendering/deferred_vert.spv");
	VkShaderModule vertModule = HelperFunctions::CreateShaderModules(vertCode);

	auto fragCode = HelperFunctions::readShaderFile(SHADERPATH"DeferredRendering/deferred_frag.spv");
	VkShaderModule fragModule = HelperFunctions::CreateShaderModules(fragCode);

	VkPipelineShaderStageCreateInfo shaderStages[] =
	{
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule),
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule),
	};

	Mesh* box = BasicShapes::getBox();
	VkDescriptorSetLayout layouts[] = { offscreenPipeline.descriptorSetLayout, box->descriptorSetLayout, box->material->descriptorSetLayout };
	auto layoutInfo = HelperFunctions::initializers::pipelineLayoutCreateInfo(3, layouts);
	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &offscreenPipeline.pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create offscreen pipeline layout");

	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	info.pInputAssemblyState = &inputAssemblyState;
	info.pVertexInputState = &vertexInputState;
	info.pColorBlendState = &colorBlendState;
	info.pDepthStencilState = &depthStencilState;
	info.pMultisampleState = &multisampleState;
	info.pDynamicState = &dynamicState;
	info.pRasterizationState = &rasterizerState;
	info.pViewportState = &viewportState;
	info.stageCount = 2;
	info.pStages = shaderStages;
	info.layout = offscreenPipeline.pipelineLayout;
	info.renderPass = offscreenPipeline.renderPass;
	info.subpass = 0;

	if (vkCreateGraphicsPipelines(device, nullptr, 1, &info, nullptr, &offscreenPipeline.pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create offscreen graphics pipeline");

	vkDestroyShaderModule(device, vertModule, nullptr);
	vkDestroyShaderModule(device, fragModule, nullptr);
}

void DeferredRendering::CreateOffscreenFramebuffers(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;
	VkImageView attachments[] = { colorTexture.imageView, offscreenPipeline.depthStencilBufferView, normalTexture.imageView, positionTexture.imageView };

	VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fbInfo.attachmentCount = 4;
	fbInfo.pAttachments = attachments;
	fbInfo.layers = 1;
	fbInfo.width = dim.width;
	fbInfo.height = dim.height;
	fbInfo.renderPass = offscreenPipeline.renderPass;

	size_t numImages = swapChain.swapChainImages.size();
	offscreenPipeline.framebuffers.resize(numImages);
	for (int i = 0; i < numImages; i++)
	{
		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &offscreenPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create one or more framebuffers");
	}
}


// COMPOSITION PIPELINE GENERATION

void DeferredRendering::CreateCompositionPipelineResources(const VulkanSwapChain& swapChain)
{
	size_t numImages = swapChain.swapChainImages.size();

	// uniform buffers
	{
		compositionPipeline.uniformBuffers.resize(numImages);
		for (int i = 0; i < numImages; i++)
		{
			HelperFunctions::createBuffer(sizeof(compositionUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				compositionPipeline.uniformBuffers[i].buffer, compositionPipeline.uniformBuffers[i].bufferMemory);
		}
		
	}

	// descriptors
	{
		VkDescriptorPoolSize poolSizes[2] = 
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,9}
		};

		VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolCreateInfo.poolSizeCount = 2;
		poolCreateInfo.pPoolSizes = poolSizes;
		poolCreateInfo.maxSets = 12; // 1 uniform buffer + 3 textures PER frame = 12 total sets
		if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &compositionPipeline.descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create composition pipeline descriptor pool");

		compositionPipeline.isDescriptorPoolEmpty = false;
		VkDescriptorSetLayoutBinding bindings[4] = {};
		
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[0].pImmutableSamplers = nullptr;
		
		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].pImmutableSamplers = nullptr;
		
		bindings[2].binding = 2;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[2].descriptorCount = 1;
		bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[2].pImmutableSamplers = nullptr;

		bindings[3].binding = 3;
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[3].descriptorCount = 1;
		bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[3].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutCreateInfo.bindingCount = 4;
		layoutCreateInfo.pBindings = bindings;
		if (vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &compositionPipeline.descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create composition pipeline descriptor set layout");

		std::vector<VkDescriptorSetLayout> layouts(numImages, compositionPipeline.descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = compositionPipeline.descriptorPool;
		allocInfo.descriptorSetCount = numImages;
		allocInfo.pSetLayouts = layouts.data();
		compositionPipeline.descriptorSets.resize(numImages);

		if (vkAllocateDescriptorSets(device, &allocInfo, compositionPipeline.descriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor set");
		
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(compositionUBO);

		VkDescriptorImageInfo imagesInfo[3] = {};
		imagesInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imagesInfo[0].imageView = colorTexture.imageView;
		imagesInfo[0].sampler = colorTexture.sampler;

		imagesInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imagesInfo[1].imageView = normalTexture.imageView;
		imagesInfo[1].sampler = normalTexture.sampler;

		imagesInfo[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imagesInfo[2].imageView = positionTexture.imageView;
		imagesInfo[2].sampler = positionTexture.sampler;
		
		VkWriteDescriptorSet writes[4] = {};

		for (int i = 0; i < numImages; i++)
		{
			// image writes
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].descriptorCount = 1;
			writes[0].dstBinding = 0;
			writes[0].dstSet = compositionPipeline.descriptorSets[i];
			writes[0].pImageInfo = &imagesInfo[0];

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].descriptorCount = 1;
			writes[1].dstBinding = 1;
			writes[1].dstSet = compositionPipeline.descriptorSets[i];
			writes[1].pImageInfo = &imagesInfo[1];

			writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[2].descriptorCount = 1;
			writes[2].dstBinding = 2;
			writes[2].dstSet = compositionPipeline.descriptorSets[i];
			writes[2].pImageInfo = &imagesInfo[2];

			// buffer write
			writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[3].descriptorCount = 1;
			writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[3].dstBinding = 3;
			writes[3].dstSet = compositionPipeline.descriptorSets[i];

			bufferInfo.buffer = compositionPipeline.uniformBuffers[i].buffer;
			writes[3].pBufferInfo = &bufferInfo;


			vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
		}
		
	}
}

void DeferredRendering::CreateCompositionRenderPass(const VulkanSwapChain& swapChain)
{
	VkAttachmentDescription attachment = {};
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment.format = swapChain.swapChainImageFormat;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference reference = {};
	reference.attachment = 0;
	reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &reference;
	subpassDesc.pDepthStencilAttachment = nullptr;

	VkSubpassDependency dependency;
	
	// wait for previous frame to finish
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo rpCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	rpCreateInfo.attachmentCount = 1;
	rpCreateInfo.pAttachments = &attachment;
	rpCreateInfo.dependencyCount = 1;
	rpCreateInfo.pDependencies = &dependency;
	rpCreateInfo.subpassCount = 1;
	rpCreateInfo.pSubpasses = &subpassDesc;
	
	if (vkCreateRenderPass(device, &rpCreateInfo, nullptr, &compositionPipeline.renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create composition render pass");

}

void DeferredRendering::CreateCompositionPipeline(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;
	VkPipelineColorBlendAttachmentState blendAttachmentState = {};

	blendAttachmentState.blendEnable = VK_FALSE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	auto dynamicState = HelperFunctions::initializers::pipelineDynamicStateCreateInfo(2, states);
	auto inputAssemblyState = HelperFunctions::initializers::pipelineInputAssemblyStateCreateInfo();
	auto vertexInputState = HelperFunctions::initializers::pipelineVertexInputStateCreateInfo();
	auto colorBlendState = HelperFunctions::initializers::pipelineColorBlendStateCreateInfo(1, blendAttachmentState);
	auto multisampleState = HelperFunctions::initializers::pipelineMultisampleStateCreateInfo();
	auto depthStencilState = HelperFunctions::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	auto rasterizerState = HelperFunctions::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT);
	auto viewportState = HelperFunctions::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

	compositionPipeline.viewport.x = 0.0f;
	compositionPipeline.viewport.y = 0.0f;
	compositionPipeline.viewport.minDepth = 0.0f;
	compositionPipeline.viewport.maxDepth = 1.0f;
	compositionPipeline.viewport.width = (float)dim.width;
	compositionPipeline.viewport.height = (float)dim.height;
	compositionPipeline.scissors.offset = { 0, 0 };
	compositionPipeline.scissors.extent = dim;
	viewportState.pViewports = &compositionPipeline.viewport;
	viewportState.pScissors = &compositionPipeline.scissors;

	auto vertCode = HelperFunctions::readShaderFile(SHADERPATH"Global/full_screen_quad_vert.spv");
	VkShaderModule vertModule = HelperFunctions::CreateShaderModules(vertCode);

	auto fragCode = HelperFunctions::readShaderFile(SHADERPATH"DeferredRendering/composition_frag.spv");
	VkShaderModule fragModule = HelperFunctions::CreateShaderModules(fragCode);

	VkPipelineShaderStageCreateInfo shaderStages[] =
	{
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule),
		HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule),
	};

	auto layoutInfo = HelperFunctions::initializers::pipelineLayoutCreateInfo(1, &compositionPipeline.descriptorSetLayout);
	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &compositionPipeline.pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create composition pipeline layout");

	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pRasterizationState = &rasterizerState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.layout = compositionPipeline.pipelineLayout;
	pipelineInfo.renderPass = compositionPipeline.renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &compositionPipeline.pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create composition pipeline");

	vkDestroyShaderModule(device, vertModule, nullptr);
	vkDestroyShaderModule(device, fragModule, nullptr);
}

void DeferredRendering::CreateCompositionFramebuffers(const VulkanSwapChain& swapChain)
{
	size_t numImages = swapChain.swapChainImages.size();
	compositionPipeline.framebuffers.resize(numImages);
	VkExtent2D dim = swapChain.swapChainDimensions;

	VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fbInfo.attachmentCount = 1;
	fbInfo.width = dim.width;
	fbInfo.height = dim.height;
	fbInfo.layers = 1;
	fbInfo.renderPass = compositionPipeline.renderPass;
	
	for (size_t i = 0; i < numImages; i++)
	{
		fbInfo.pAttachments = &swapChain.swapChainImageViews[i];
		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &compositionPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one of more framebuffers");
	}
}






