#include "ShadowMap.h"
#include "Renderer/Loaders.h"

VkDevice device = nullptr;

ShadowMap::ShadowMap()
{
	sceneName = "";
}

ShadowMap::ShadowMap(std::string name, const VulkanSwapChain& swapChain)
{
	device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	sceneName = name;

	CreateSceneObjects();
	CreateUniforms(swapChain);

	CreateSyncObjects(swapChain);

	CreateShadowResources();
	CreateShadowRenderPass(swapChain);
	CreateShadowFramebuffers(swapChain);
	CreateShadowDescriptorSets(swapChain);

	CreateDebugResources(swapChain);

	CreateSceneRenderPass(swapChain);
	CreateSceneFramebuffers(swapChain);
	CreateSceneDescriptorSets(swapChain);

	CreatePipelines(swapChain);
	CreateCommandBuffers();

	ui = new UI(commandPool, swapChain, graphicsPipeline, VK_SAMPLE_COUNT_8_BIT);

}

ShadowMap::~ShadowMap()
{
	DestroyScene(false);
	delete ui;
}

// ********* RENDERING  ***************

void ShadowMap::DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
	ground.draw(commandBuffer, pipelineLayout, useMaterial);
	cube.draw(commandBuffer, pipelineLayout, useMaterial);
	sphere.draw(commandBuffer, pipelineLayout, useMaterial);
	monkey.draw(commandBuffer, pipelineLayout, useMaterial);
}

void ShadowMap::DrawUI(uint32_t frameIndex)
{
	static Camera* camera = Camera::GetCamera();

	ui->NewUIFrame();

	ui->NewWindow("Application");
	{
		glm::vec3 camPos = camera->GetCameraPosition();
		ui->DisplayFPS();
		ui->DrawUITextVec3("Camera Position", camPos);
	}
	ui->EndWindow();

	ui->NewWindow("Scene Data"); // TO DO: as always, figure out why depth values aren't rendered properly to texture
	{ 
		glm::vec3 lightPosition = light.getLightPos();
		ui->DrawSliderVec3("Light Position", &lightPosition, -5.0f, 5.0f);
		light.setLightPos(lightPosition);

		ui->AddSpacing(2);

		ui->DrawImage("Light Depth Texture", debugTex, &shadowSampler, glm::vec2(256));
	}
	ui->EndWindow();

	ui->EndFrame();
	ui->RenderFrame(commandBuffersList[frameIndex], frameIndex);
}

VulkanReturnValues ShadowMap::PresentScene(const VulkanSwapChain& swapChain)
{
	static auto queues = VulkanDevice::GetVulkanDevice()->GetQueues();

	uint32_t imageIndex;

	/*
	********** PREPARATION ************
	*/
	graphicsPipeline.result = vkAcquireNextImageKHR(device, swapChain.swapChain,
		UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS && graphicsPipeline.result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("Failed to acquire swap chain image");

	// check if a previous frame is using this image
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

	UpdateUniforms(imageIndex); // update matrices as needed
	RecordCommandBuffers(imageIndex);

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
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	graphicsPipeline.result = vkQueuePresentKHR(queues.presentQueue, &presentInfo);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || graphicsPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

// ********* USER INPUT ***************

void ShadowMap::HandleKeyboardInput(const uint8_t* keystates, float dt)
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

void ShadowMap::HandleMouseInput(uint32_t buttons, const int x, const int y, float mouseWheelX, float mouseWheelY)
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

// ********* SCENE SETUP **************

void ShadowMap::RecordScene()
{
	
}

void ShadowMap::RecordCommandBuffers(uint32_t index)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.renderArea.offset = { 0, 0 };

	VkClearValue clearValues[2];
	VkDeviceSize offsets[] = { 0 };

	if (vkBeginCommandBuffer(commandBuffersList[index], &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to being recording command buffer!");

	// perform shadow pass to generate shadow map
	{
		clearValues[0].depthStencil = {1.0f, 0};
		renderPassInfo.renderPass = shadowPipeline.renderPass;
		renderPassInfo.framebuffer = shadowPipeline.framebuffers[0];
		renderPassInfo.renderArea.extent = shadowPipeline.scissors.extent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(commandBuffersList[index], 0, 1, &shadowPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &shadowPipeline.scissors);
		vkCmdSetDepthBias(commandBuffersList[index], depthBiasConstant, 0.0f, depthBiasSlope);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipelineLayout,
			0, 1, &shadowPipeline.descriptorSets[index], 0, nullptr);

		// the floor doesn't cast shadows, so no need to render it here
		DrawScene(commandBuffersList[index], shadowPipeline.pipelineLayout);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	// run fsq shaders to write depth map to an offscreen texture
	{
		clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};

		renderPassInfo.renderPass = debugPipeline.renderPass;
		renderPassInfo.framebuffer = debugPipeline.framebuffers[0];
		renderPassInfo.renderArea.extent = debugPipeline.scissors.extent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(commandBuffersList[index], 0, 1, &debugPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &debugPipeline.scissors);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline.pipelineLayout,
			0, 1, &debugPipeline.descriptorSets[0], 0, nullptr);

		float push[] = { light.getNearPlane(), light.getFarPlane() };
		vkCmdPushConstants(commandBuffersList[index], debugPipeline.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(float), push);

		vkCmdDraw(commandBuffersList[index], 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	// render the scene normally, rendering the depth map to a UI image
	{
		VkClearValue clearColors[2] = {};
		clearColors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColors[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.renderPass = graphicsPipeline.renderPass;
		renderPassInfo.framebuffer = graphicsPipeline.framebuffers[index];
		renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;

		vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(commandBuffersList[index], 0, 1, &graphicsPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &graphicsPipeline.scissors);

		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
			0, 1, &graphicsPipeline.descriptorSets[index], 0, nullptr);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);

		DrawScene(commandBuffersList[index], graphicsPipeline.pipelineLayout, true);

		DrawUI(index);
		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	if (vkEndCommandBuffer(commandBuffersList[index]) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer");
}

void ShadowMap::RecreateScene(const VulkanSwapChain& swapChain)
{
	DestroyScene(true);

	CreateUniforms(swapChain);

	CreateShadowResources();
	CreateShadowRenderPass(swapChain);
	CreateShadowFramebuffers(swapChain);
	CreateShadowDescriptorSets(swapChain);

	CreateDebugResources(swapChain);

	CreateSceneRenderPass(swapChain);
	CreateSceneFramebuffers(swapChain);
	CreateSceneDescriptorSets(swapChain);

	CreatePipelines(swapChain);
	CreateCommandBuffers();

	delete ui;
	ui = new UI(commandPool, swapChain, graphicsPipeline, VK_SAMPLE_COUNT_8_BIT);
}

void ShadowMap::DestroyScene(bool isRecreation)
{
	graphicsPipeline.destroyGraphicsPipeline(device);
	debugPipeline.destroyGraphicsPipeline(device);
	shadowPipeline.destroyGraphicsPipeline(device);

	msaaTex.destroyTexture();
	debugTex.destroyTexture();
	vkDestroySampler(device, shadowSampler, nullptr);

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

		cube.destroyMesh();
		ground.destroyMesh();
		monkey.destroyMesh();
		sphere.destroyMesh();
	}
}

void ShadowMap::CreateSceneObjects()
{
	// create meshes
	glm::mat4 model = glm::mat4(1.0f);

	cube = BasicShapes::createBox();
	cube.setMaterialWithPreset(MaterialPresets::POLISHED_GOLD);
	model = glm::translate(glm::vec3(-1.0f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.5f));
	cube.setModelMatrix(model);

	ground = BasicShapes::createPlane();
	ground.setMaterialWithPreset(MaterialPresets::EMERALD);
	model = glm::scale(glm::vec3(5.0f));
	ground.setModelMatrix(model);

	monkey = BasicShapes::createMonkey();
	monkey.setMaterialWithPreset(MaterialPresets::TURQUOISE);
	model = glm::translate(glm::vec3(0.0f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.5f));
	monkey.setModelMatrix(model);


	sphere = BasicShapes::createSphere();
	sphere.setMaterialWithPreset(MaterialPresets::OBSIDIAN);
	model = glm::translate(glm::vec3(1.0f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.5f));
	sphere.setModelMatrix(model);
}

void ShadowMap::CreateUniforms(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;

	light = SpotLight(glm::vec3(0.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(1.0f), 0.1f, 100.0f);

	uboShadow.viewProj = light.getLightProj() * light.getLightView();

	Camera* camera = Camera::GetCamera();
	uboScene.view = camera->GetViewMatrix();
	uboScene.proj = glm::perspective(glm::radians(45.0f), float(dim.width) / float(dim.height), 0.1f, 1000.0f);
	uboScene.proj[1][1] *= -1;
	uboScene.lightVP = uboShadow.viewProj;
	uboScene.lightPos = glm::vec4(light.getLightPos(), 1.0);
}

void ShadowMap::UpdateUniforms(uint32_t index)
{
	uboShadow.viewProj = light.getLightProj() * light.getLightView();
	glm::vec3 lightPos = light.getLightPos();

	static Camera* camera = Camera::GetCamera();
	uboScene.view = camera->GetViewMatrix();
	uboScene.lightVP = uboShadow.viewProj;
	uboScene.lightPos = glm::vec4(lightPos, 1.0f);

	// copy data to uniform buffers
	graphicsPipeline.uniformBuffers[index].map();
	shadowPipeline.uniformBuffers[index].map();

	memcpy(graphicsPipeline.uniformBuffers[index].mappedMemory, &uboScene, sizeof(uboScene));
	memcpy(shadowPipeline.uniformBuffers[index].mappedMemory, &uboShadow, sizeof(uboShadow));

	graphicsPipeline.uniformBuffers[index].unmap();
	shadowPipeline.uniformBuffers[index].unmap();
}


void ShadowMap::CreatePipelines(const VulkanSwapChain& swapChain)
{
#pragma region SETUP

	// vertex descriptions
	VkVertexInputBindingDescription bindingDescription = ModelVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescription = ModelVertex::getAttributeDescriptions();

	VkPipelineColorBlendAttachmentState colorBlendingAttachment = 
	{
		VK_TRUE, 
		VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};


	VkPipelineVertexInputStateCreateInfo vertexInputInfo = HelperFunctions::initializers::pipelineVertexInputStateCreateInfo(1, bindingDescription, 3, attributeDescription.data());
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = HelperFunctions::initializers::pipelineInputAssemblyStateCreateInfo();
	VkPipelineViewportStateCreateInfo viewportState = HelperFunctions::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineRasterizationStateCreateInfo rasterizerState = HelperFunctions::initializers::pipelineRasterizationStateCreateInfo();
	VkPipelineMultisampleStateCreateInfo multisampleState = HelperFunctions::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_8_BIT);
	VkPipelineColorBlendStateCreateInfo colorBlendState = HelperFunctions::initializers::pipelineColorBlendStateCreateInfo(1, colorBlendingAttachment);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = HelperFunctions::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

	VkDynamicState dynamicStateEnables[3] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS};
	VkPipelineDynamicStateCreateInfo dynamicState = HelperFunctions::initializers::pipelineDynamicStateCreateInfo(2, dynamicStateEnables);

	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerState;
	pipelineInfo.pMultisampleState = &multisampleState; 
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkExtent2D dim = swapChain.swapChainDimensions;

#pragma endregion

#pragma region SCENE_PIPELINE
	{
		auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/scene_vert.spv");
		VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

		auto fragShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/scene_frag.spv");
		VkShaderModule fragShaderModule = HelperFunctions::CreateShaderModules(fragShaderCode);

		VkPipelineShaderStageCreateInfo shaderStages[] =
		{
			HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
			HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule),
		};

		// models and shapes have the same descriptor set layout
		VkDescriptorSetLayout layouts[3] = { graphicsPipeline.descriptorSetLayout, cube.descriptorSetLayout, cube.material->descriptorSetLayout };

		VkPipelineLayoutCreateInfo layoutInfo = HelperFunctions::initializers::pipelineLayoutCreateInfo(3, layouts);

		graphicsPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout);
		if (graphicsPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout");

		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0.8f;

		graphicsPipeline.viewport.x = 0.0f;
		graphicsPipeline.viewport.y = 0.0f;
		graphicsPipeline.viewport.minDepth = 0.0f;
		graphicsPipeline.viewport.maxDepth = 1.0f;
		graphicsPipeline.viewport.width = (float)dim.width;
		graphicsPipeline.viewport.height = (float)dim.height;
		graphicsPipeline.scissors.offset = { 0, 0 };
		graphicsPipeline.scissors.extent = dim;
		viewportState.pViewports = &graphicsPipeline.viewport;
		viewportState.pScissors = &graphicsPipeline.scissors;


		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = graphicsPipeline.renderPass;
		pipelineInfo.layout = graphicsPipeline.pipelineLayout;
		pipelineInfo.pViewportState = &viewportState;

		graphicsPipeline.result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline.pipeline);
		if (graphicsPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}
#pragma endregion

#pragma region SHADOW_PIPELINE
	{
		auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/shadow_vert.spv");
		VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

		VkPipelineShaderStageCreateInfo shaderStage = HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule);

		// no blend attachment states
		colorBlendState.attachmentCount = 0;
		colorBlendState.pAttachments = nullptr;
		
		rasterizerState.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizerState.depthBiasEnable = VK_TRUE;
		colorBlendingAttachment.blendEnable = VK_FALSE;

		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.minSampleShading = 0.0f;

		// viewport
		shadowPipeline.viewport.x = 0.0f;
		shadowPipeline.viewport.y = 0.0f;
		shadowPipeline.viewport.minDepth = 0.0f;
		shadowPipeline.viewport.maxDepth = 1.0f;
		shadowPipeline.viewport.width = (float)shadowMapDim;
		shadowPipeline.viewport.height = (float)shadowMapDim;
		shadowPipeline.scissors.offset = { 0, 0 };
		shadowPipeline.scissors.extent = { shadowMapDim, shadowMapDim };
		viewportState.pViewports = &shadowPipeline.viewport;
		viewportState.pScissors = &shadowPipeline.scissors;


		// dynamic state
		dynamicState.dynamicStateCount = 3;

		// ** Pipeline Layout ** 
		VkDescriptorSetLayout layouts[] = { shadowPipeline.descriptorSetLayout, cube.descriptorSetLayout };

		VkPipelineLayoutCreateInfo layoutInfo = HelperFunctions::initializers::pipelineLayoutCreateInfo(2, layouts);

		shadowPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &shadowPipeline.pipelineLayout);
		if (shadowPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout");

		pipelineInfo.renderPass = shadowPipeline.renderPass;
		pipelineInfo.stageCount = 1;
		pipelineInfo.pStages = &shaderStage;
		pipelineInfo.renderPass = shadowPipeline.renderPass;
		pipelineInfo.layout = shadowPipeline.pipelineLayout;

		shadowPipeline.result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &shadowPipeline.pipeline);
		if (shadowPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}
#pragma endregion

#pragma region DEBUG_PIPELINE
	{
		auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"Global/full_screen_quad.spv");
		VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

		auto fragShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/debug_frag.spv");
		VkShaderModule fragShaderModule = HelperFunctions::CreateShaderModules(fragShaderCode);

		VkPipelineShaderStageCreateInfo shaderStages[] =
		{
			HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
			HelperFunctions::initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
		};


		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;

		// no blend attachment states
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendingAttachment;

		rasterizerState.depthBiasEnable = VK_FALSE;
		colorBlendingAttachment.blendEnable = VK_TRUE;

		// viewport
		debugPipeline.viewport.x = 0.0f;
		debugPipeline.viewport.y = 0.0f;
		debugPipeline.viewport.minDepth = 0.0f;
		debugPipeline.viewport.maxDepth = 1.0f;
		debugPipeline.viewport.width = shadowMapDim;
		debugPipeline.viewport.height = shadowMapDim;
		debugPipeline.scissors.offset = { 0, 0 };
		debugPipeline.scissors.extent = { shadowMapDim, shadowMapDim };
		viewportState.pViewports = &debugPipeline.viewport;
		viewportState.pScissors = &debugPipeline.scissors;


		// dynamic state
		dynamicState.dynamicStateCount = 0;
		pipelineInfo.pDynamicState = nullptr;

		VkPushConstantRange push = {};
		push.offset = 0;
		push.size = 2 * sizeof(float);
		push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkPipelineLayoutCreateInfo layoutInfo = HelperFunctions::initializers::pipelineLayoutCreateInfo(1, &debugPipeline.descriptorSetLayout, 1, &push);

		vkCreatePipelineLayout(device, &layoutInfo, nullptr, &debugPipeline.pipelineLayout);

		pipelineInfo.renderPass = debugPipeline.renderPass;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = debugPipeline.renderPass;
		pipelineInfo.layout = debugPipeline.pipelineLayout;

		vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &debugPipeline.pipeline);

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}
#pragma endregion
}

void ShadowMap::CreateSyncObjects(const VulkanSwapChain& swapChain)
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
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentCompleteSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create sync objects for a frame");
	}
}

void ShadowMap::CreateCommandBuffers()
{
	commandBuffersList.resize(graphicsPipeline.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffersList.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffersList.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers");
}


// ********* SHADOW PIPELINE **************

void ShadowMap::CreateShadowRenderPass(const VulkanSwapChain& swapChain)
{
	VkAttachmentDescription depthAttachmentDescription = {};
	depthAttachmentDescription.format = depthFormat;
	depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Attachment will be transitioned to shader read at render pass end
	depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 0;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pColorAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	// wait for scene render pass to finish reading from depth buffer
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// scene pass must wait for the shadow pass to write to the depth buffer
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &depthAttachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 2;
	renderPassCreateInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &shadowPipeline.renderPass))
		throw std::runtime_error("Failed to create shadow render pass");
}

void ShadowMap::CreateShadowDescriptorSets(const VulkanSwapChain& swapChain)
{
	// shadow pass only needs a vertex shader uniform buffer for light vp

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.maxSets = 3;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &shadowPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	shadowPipeline.isDescriptorPoolEmpty = false;

	VkDescriptorSetLayoutBinding shadowUBO = {};
	shadowUBO.binding = 0;
	shadowUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadowUBO.descriptorCount = 1;
	shadowUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	shadowUBO.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &shadowUBO;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &shadowPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow pass descriptor set layout");

	size_t swapChainSize = swapChain.swapChainImages.size();
	shadowPipeline.uniformBuffers.resize(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++)
	{
		HelperFunctions::createBuffer(sizeof(shadowUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			shadowPipeline.uniformBuffers[i].buffer, shadowPipeline.uniformBuffers[i].bufferMemory);

		shadowPipeline.uniformBuffers[i].map();
		memcpy(shadowPipeline.uniformBuffers[i].mappedMemory, &uboShadow, sizeof(uboShadow));
		shadowPipeline.uniformBuffers[i].unmap();
	}


	// pool set layouts

	std::vector<VkDescriptorSetLayout> layout(swapChainSize, shadowPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = shadowPipeline.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	allocInfo.pSetLayouts = layout.data();
	shadowPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &allocInfo, shadowPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

	// writes

	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = shadowPipeline.uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(shadowUBO);

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = shadowPipeline.descriptorSets[i];
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}
}

void ShadowMap::CreateShadowFramebuffers(const VulkanSwapChain& swapChain)
{
	VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	info.renderPass = shadowPipeline.renderPass;
	info.attachmentCount = 1;
	info.pAttachments = &shadowPipeline.depthStencilBufferView;
	info.width = shadowMapDim;
	info.height = shadowMapDim;
	info.layers = 1;

	shadowPipeline.framebuffers.resize(1);

	if (vkCreateFramebuffer(device, &info, nullptr, &shadowPipeline.framebuffers[0]) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow pass framebuffer");

}

void ShadowMap::CreateShadowResources()
{
	VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkFilter filter = VK_FILTER_LINEAR;

	HelperFunctions::createImage(shadowMapDim, shadowMapDim, 1, 1,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		shadowPipeline.depthStencilBuffer, shadowPipeline.depthStencilBufferMemory);

	HelperFunctions::createImageView(shadowPipeline.depthStencilBuffer,
		shadowPipeline.depthStencilBufferView, depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	shadowPipeline.isDepthBufferEmpty = false;

	// create sampler
	VkSamplerCreateInfo sampler = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampler.minFilter = filter;
	sampler.magFilter = filter;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = addressMode;
	sampler.addressModeV = addressMode;
	sampler.addressModeW = addressMode;
	sampler.anisotropyEnable = VK_FALSE;
	sampler.maxAnisotropy = 1.0f;
	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (vkCreateSampler(device, &sampler, nullptr, &shadowSampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow map sampler");
}


// ********* DEBUG PIPELINE ***************

void ShadowMap::CreateDebugResources(const VulkanSwapChain& swapChain)
{
	// descriptors
	{
		VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &debugPipeline.descriptorPool);

		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &binding;
		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &debugPipeline.descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = debugPipeline.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &debugPipeline.descriptorSetLayout;

		debugPipeline.descriptorSets.resize(1);
		vkAllocateDescriptorSets(device, &allocInfo, &debugPipeline.descriptorSets[0]);


		VkDescriptorImageInfo info = {};
		info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		info.imageView = shadowPipeline.depthStencilBufferView;
		info.sampler = shadowSampler;

		VkWriteDescriptorSet write = HelperFunctions::initializers::writeDescriptorSet(debugPipeline.descriptorSets[0], &info);
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

		debugPipeline.isDescriptorPoolEmpty = false;
	}

	// render pass
	{
		VkAttachmentDescription colorDescription = {};
		colorDescription.format = swapChain.swapChainImageFormat;
		colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		colorDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pDepthStencilAttachment = nullptr;

		VkSubpassDependency dependencies[2];

		// scene pass must wait for debug pass to finish reading from the shadow map
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// shadow pass waits for render pass fragment shader to finish 
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorDescription;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies;

		vkCreateRenderPass(device, &renderPassInfo, nullptr, &debugPipeline.renderPass);
	}

	// framebuffer
	{
		VkImageView attachment = {};

		HelperFunctions::createImage(shadowMapDim, shadowMapDim, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
			swapChain.swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, debugTex.image, debugTex.imageMemory);

		HelperFunctions::createImageView(debugTex.image, debugTex.imageView, 
			swapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_VIEW_TYPE_2D, 1);

		VkFramebufferCreateInfo sceneFramebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		sceneFramebufferInfo.renderPass = debugPipeline.renderPass;
		sceneFramebufferInfo.attachmentCount = 1;
		sceneFramebufferInfo.pAttachments = &debugTex.imageView;
		sceneFramebufferInfo.width = shadowMapDim;
		sceneFramebufferInfo.height = shadowMapDim;
		sceneFramebufferInfo.layers = 1;

		debugPipeline.framebuffers.resize(1);
		vkCreateFramebuffer(device, &sceneFramebufferInfo, nullptr, &debugPipeline.framebuffers[0]);
	}
}

// ********* SCENE PIPELINE ***************

void ShadowMap::CreateSceneDescriptorSets(const VulkanSwapChain& swapChain)
{
#pragma region POOLS
	size_t swapChainSize = swapChain.swapChainImages.size();
	uint32_t descriptorCount = (uint32_t)swapChainSize; // 1 descriptor set per frame

	VkDescriptorPoolSize poolSizes[2] = 
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
	};

	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	graphicsPipeline.isDescriptorPoolEmpty = false;
#pragma endregion

#pragma region SET_LAYOUT
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	// scene uniform buffer
	VkDescriptorSetLayoutBinding sceneUBOBinding = {};
	sceneUBOBinding.binding = 0;
	sceneUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sceneUBOBinding.descriptorCount = 1;
	sceneUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	sceneUBOBinding.pImmutableSamplers = nullptr;
	bindings.push_back(sceneUBOBinding);

	// shadow pass depth textures
	VkDescriptorSetLayoutBinding shadowMapBinding = {};
	shadowMapBinding.binding = 1;
	shadowMapBinding.descriptorCount = 1;
	shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowMapBinding.pImmutableSamplers = nullptr;
	shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings.push_back(shadowMapBinding);

	VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");
#pragma endregion

#pragma region ALLOC_SETS
	// create uniform buffers for graphics pipeline
	VkDeviceSize bufferSize = sizeof(uboScene);
	graphicsPipeline.uniformBuffers.resize(swapChainSize);

	for (size_t i = 0; i < swapChainSize; i++)
	{
		HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			graphicsPipeline.uniformBuffers[i].buffer, graphicsPipeline.uniformBuffers[i].bufferMemory);

		vkMapMemory(device, graphicsPipeline.uniformBuffers[i].bufferMemory, 0, sizeof(uboScene), 0, &graphicsPipeline.uniformBuffers[i].mappedMemory);
		memcpy(graphicsPipeline.uniformBuffers[i].mappedMemory, &uboScene, sizeof(uboScene));
		vkUnmapMemory(device, graphicsPipeline.uniformBuffers[i].bufferMemory);
	}


	std::vector<VkDescriptorSetLayout> layout(swapChainSize, graphicsPipeline.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	allocInfo.pSetLayouts = layout.data();

	graphicsPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

#pragma endregion

#pragma region WRITE_DESCRIPTORS

	for (size_t i = 0; i < swapChainSize; i++)
	{
		// scene descriptors
		VkDescriptorBufferInfo uboInfo = {};
		uboInfo.buffer = graphicsPipeline.uniformBuffers[i].buffer;
		uboInfo.offset = 0;
		uboInfo.range = sizeof(uboScene);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		imageInfo.imageView = shadowPipeline.depthStencilBufferView;
		imageInfo.sampler = shadowSampler;

		VkWriteDescriptorSet writeDescriptors[2] =
		{
			HelperFunctions::initializers::writeDescriptorSet(graphicsPipeline.descriptorSets[i], &uboInfo),
			HelperFunctions::initializers::writeDescriptorSet(graphicsPipeline.descriptorSets[i], &imageInfo, 1)
		};

		vkUpdateDescriptorSets(device, 2, writeDescriptors, 0, nullptr);
	}
#pragma endregion
}

void ShadowMap::CreateSceneFramebuffers(const VulkanSwapChain& swapChain)
{
	graphicsPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());
	VkExtent2D dim = swapChain.swapChainDimensions;

	HelperFunctions::createImage(dim.width, dim.height, 1, 1, VK_SAMPLE_COUNT_8_BIT,
		VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferMemory);

	HelperFunctions::createImageView(graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferView,
		VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	graphicsPipeline.isDepthBufferEmpty = false;

	// multisampling resolve texture

	HelperFunctions::createImage(dim.width, dim.height, 1, 1, VK_SAMPLE_COUNT_8_BIT,
		VK_IMAGE_TYPE_2D, swapChain.swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, msaaTex.image, msaaTex.imageMemory);

	HelperFunctions::createImageView(msaaTex.image, msaaTex.imageView,
		swapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	VkImageView attachments[3];

	VkFramebufferCreateInfo sceneFramebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	sceneFramebufferInfo.renderPass = graphicsPipeline.renderPass;
	sceneFramebufferInfo.attachmentCount = 3;
	sceneFramebufferInfo.pAttachments = attachments;
	sceneFramebufferInfo.width = dim.width;
	sceneFramebufferInfo.height = dim.height;
	sceneFramebufferInfo.layers = 1;

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		attachments[0] = msaaTex.imageView;
		attachments[1] = graphicsPipeline.depthStencilBufferView;
		attachments[2] = swapChain.swapChainImageViews[i];

		if (vkCreateFramebuffer(device, &sceneFramebufferInfo, nullptr, &graphicsPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}
}

void ShadowMap::CreateSceneRenderPass(const VulkanSwapChain& swapChain)
{
	VkAttachmentDescription sceneColorAttachment = {};
	sceneColorAttachment.format = swapChain.swapChainImageFormat;
	sceneColorAttachment.flags = 0;
	sceneColorAttachment.samples = VK_SAMPLE_COUNT_8_BIT;
	sceneColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	sceneColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	sceneColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	sceneColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	sceneColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	sceneColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference sceneColorReference = {};
	sceneColorReference.attachment = 0;
	sceneColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth attachment
	VkAttachmentDescription sceneDepthAttachment = {};
	sceneDepthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	sceneDepthAttachment.flags = 0;
	sceneDepthAttachment.samples = VK_SAMPLE_COUNT_8_BIT;
	sceneDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	sceneDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	sceneDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	sceneDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	sceneDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	sceneDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference sceneDepthReference = {};
	sceneDepthReference.attachment = 1;
	sceneDepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// multisampling resolve attachment
	VkAttachmentDescription multisamplingResolveAttachment = {};
	multisamplingResolveAttachment.format = swapChain.swapChainImageFormat;
	multisamplingResolveAttachment.flags = 0;
	multisamplingResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	multisamplingResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	multisamplingResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	multisamplingResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	multisamplingResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	multisamplingResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference multisamplingResolveReference = {};
	multisamplingResolveReference.attachment = 2;
	multisamplingResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &sceneColorReference;
	subpass.pDepthStencilAttachment = &sceneDepthReference;
	subpass.pResolveAttachments = &multisamplingResolveReference;

	std::array<VkSubpassDependency, 3> dependencies;

	// scene pass must wait for debug pass to finish reading from the shadow map
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// wait for multisampling for finish
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = 0;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// shadow pass waits for render pass fragment shader to finish 
	dependencies[2].srcSubpass = 0;
	dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkAttachmentDescription attachments[] = { sceneColorAttachment, sceneDepthAttachment, multisamplingResolveAttachment };

	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = 3;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 3;
	renderPassInfo.pDependencies = dependencies.data();

	graphicsPipeline.result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &graphicsPipeline.renderPass);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}
