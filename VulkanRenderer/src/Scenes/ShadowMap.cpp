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

	CreateUniforms(swapChain);

	CreateSyncObjects(swapChain);

	CreateShadowRenderPass(swapChain);
	CreateShadowFramebuffers(swapChain);
	CreateShadowDescriptorSets(swapChain);

	CreateSceneRenderPass(swapChain);
	CreateSceneFramebuffers(swapChain);
	CreateSceneDescriptorSets(swapChain);

	CreatePipelines(swapChain);
	CreateCommandBuffers();

	//RecordScene();
}

ShadowMap::~ShadowMap()
{
	DestroyScene(false);

}

// ********* RENDERING  ***************

void ShadowMap::DrawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
	BasicShapes::drawPlane(commandBuffer, pipelineLayout, useMaterial);
	BasicShapes::drawBox(commandBuffer, pipelineLayout, useMaterial);
	BasicShapes::drawSphere(commandBuffer, pipelineLayout, useMaterial);
	BasicShapes::drawMonkey(commandBuffer, pipelineLayout, useMaterial);
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

	if (debug == 0)
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

	if (keystates[SDL_SCANCODE_LEFT])
	{
		debug = 0;
		return;
	}

	if (keystates[SDL_SCANCODE_RIGHT])
	{
		debug = 1;
		return;
	}

	if (keystates[SDL_SCANCODE_UP])
	{
		animate = true;
		return;
	}
	
	else if (keystates[SDL_SCANCODE_DOWN])
	{
		animate = false;
		return;
	}
}

void ShadowMap::HandleMouseInput(const int x, const int y)
{
	if (debug == 1) return;
	static Camera* camera = Camera::GetCamera();

	static float deltaX = 0.0f;
	static float deltaY = 0.0f;

	// check if current motion is less than/greater than last motion
	float sensivity = 0.1f;

	deltaX = x * sensivity;
	deltaY = y * sensivity;

	camera->RotateCamera(deltaX, deltaY);
}

// ********* SCENE SETUP **************

void ShadowMap::RecordScene()
{
	for (size_t i = 0; i < commandBuffersList.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // defines how we want to use the command buffer
		beginInfo.pInheritanceInfo = nullptr; // only important if we're using secondary command buffers

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderArea.offset = { 0, 0 };

		VkClearValue clearColors[2] = {};
		clearColors[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clearColors[1].depthStencil = { 1.0f, 0 };

		VkDeviceSize offsets[] = { 0 };

		if (vkBeginCommandBuffer(commandBuffersList[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to being recording command buffer!");

		// perform shadow pass to generate shadow map
		{
			renderPassInfo.renderPass = shadowPipeline.renderPass;
			renderPassInfo.framebuffer = shadowPipeline.framebuffers[0];
			renderPassInfo.renderArea.extent = shadowPipeline.scissors.extent;
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColors[1];

			vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(commandBuffersList[i], 0, 1, &shadowPipeline.viewport);
			vkCmdSetScissor(commandBuffersList[i], 0, 1, &shadowPipeline.scissors);
			vkCmdSetDepthBias(commandBuffersList[i], depthBiasConstant, 0.0f, depthBiasSlope);

			vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipeline);
			vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipelineLayout,
				0, 1, &shadowPipeline.descriptorSets[i], 0, nullptr);

			// the floor doesn't cast shadows, so no need to render it here
			BasicShapes::drawBox(commandBuffersList[i], shadowPipeline.pipelineLayout);
			BasicShapes::drawSphere(commandBuffersList[i], shadowPipeline.pipelineLayout);
			BasicShapes::drawMonkey(commandBuffersList[i], shadowPipeline.pipelineLayout);

			vkCmdEndRenderPass(commandBuffersList[i]);
		}
		
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;

		// render the scene normally
		{
			renderPassInfo.renderPass = graphicsPipeline.renderPass;
			renderPassInfo.framebuffer = graphicsPipeline.framebuffers[i];
			renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;

			vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(commandBuffersList[i], 0, 1, &graphicsPipeline.viewport);
			vkCmdSetScissor(commandBuffersList[i], 0, 1, &graphicsPipeline.scissors);

			vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
				0, 1, &graphicsPipeline.descriptorSets[i], 0, nullptr);

			vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (debug == 0) ? graphicsPipeline.pipeline : debugPipeline.pipeline);

			DrawScene(commandBuffersList[i], graphicsPipeline.pipelineLayout, true);

			vkCmdEndRenderPass(commandBuffersList[i]);
		}

		clearColors->color = { 0.0f, 0.0f, 0.0f, 1.0f };
		// render the shadow map to a quad
		{
			renderPassInfo.renderPass = quadPipeline.renderPass;
			renderPassInfo.framebuffer = quadPipeline.framebuffers[i];
			renderPassInfo.renderArea.extent = quadPipeline.scissors.extent;

			vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(commandBuffersList[i], 0, 1, &quadPipeline.viewport);
			vkCmdSetScissor(commandBuffersList[i], 0, 1, &quadPipeline.scissors);

			vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, quadPipeline.pipeline);
			vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, quadPipeline.pipelineLayout,
				0, 1, &quadPipeline.descriptorSets[i], 0, nullptr);

			vkCmdDraw(commandBuffersList[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffersList[i]);
		}

		if (vkEndCommandBuffer(commandBuffersList[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
	}
}

void ShadowMap::RecordCommandBuffers(uint32_t index)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // defines how we want to use the command buffer
	beginInfo.pInheritanceInfo = nullptr; // only important if we're using secondary command buffers

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };

	VkClearValue clearColors[2] = {};
	clearColors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[1].depthStencil = { 1.0f, 0 };

	VkClearValue clear = {};
	VkDeviceSize offsets[] = { 0 };

	if (vkBeginCommandBuffer(commandBuffersList[index], &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to being recording command buffer!");

	// perform shadow pass to generate shadow map
	{
		renderPassInfo.renderPass = shadowPipeline.renderPass;
		renderPassInfo.framebuffer = shadowPipeline.framebuffers[0];
		renderPassInfo.renderArea.extent = shadowPipeline.scissors.extent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColors[1];

		vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(commandBuffersList[index], 0, 1, &shadowPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &shadowPipeline.scissors);
		vkCmdSetDepthBias(commandBuffersList[index], depthBiasConstant, 0.0f, depthBiasSlope);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline.pipelineLayout,
			0, 1, &shadowPipeline.descriptorSets[index], 0, nullptr);

		// the floor doesn't cast shadows, so no need to render it here
		DrawScene(commandBuffersList[index], shadowPipeline.pipelineLayout);
		//BasicShapes::drawBox(commandBuffersList[index], shadowPipeline.pipelineLayout);
		//BasicShapes::drawSphere(commandBuffersList[index], shadowPipeline.pipelineLayout);
		//BasicShapes::drawMonkey(commandBuffersList[index], shadowPipeline.pipelineLayout);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearColors;

	// render the scene normally
	{
		renderPassInfo.renderPass = graphicsPipeline.renderPass;
		renderPassInfo.framebuffer = graphicsPipeline.framebuffers[index];
		renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;

		vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(commandBuffersList[index], 0, 1, &graphicsPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &graphicsPipeline.scissors);

		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
			0, 1, &graphicsPipeline.descriptorSets[index], 0, nullptr);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, (debug == 0) ? graphicsPipeline.pipeline : debugPipeline.pipeline);

		DrawScene(commandBuffersList[index], graphicsPipeline.pipelineLayout, true);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	clearColors->color = { 0.0f, 0.0f, 0.0f, 1.0f };
	float push[] = { light.getNearPlane(), light.getFarPlane() };
	// render the shadow map to a quad
	{
		renderPassInfo.renderPass = quadPipeline.renderPass;
		renderPassInfo.framebuffer = quadPipeline.framebuffers[index];
		renderPassInfo.renderArea.extent = quadPipeline.scissors.extent;

		vkCmdBeginRenderPass(commandBuffersList[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(commandBuffersList[index], 0, 1, &quadPipeline.viewport);
		vkCmdSetScissor(commandBuffersList[index], 0, 1, &quadPipeline.scissors);

		vkCmdBindPipeline(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, quadPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[index], VK_PIPELINE_BIND_POINT_GRAPHICS, quadPipeline.pipelineLayout,
			0, 1, &quadPipeline.descriptorSets[index], 0, nullptr);

		vkCmdPushConstants(commandBuffersList[index], quadPipeline.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(float), push);
		vkCmdDraw(commandBuffersList[index], 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffersList[index]);
	}

	if (vkEndCommandBuffer(commandBuffersList[index]) != VK_SUCCESS)
		throw std::runtime_error("Failed to record command buffer");
}

void ShadowMap::RecreateScene(const VulkanSwapChain& swapChain)
{
	DestroyScene(true);

	CreateShadowRenderPass(swapChain);
	CreateShadowFramebuffers(swapChain);
	CreateShadowDescriptorSets(swapChain);

	CreateSceneRenderPass(swapChain);
	CreateSceneFramebuffers(swapChain);
	CreateSceneDescriptorSets(swapChain);

	CreatePipelines(swapChain);
	CreateCommandBuffers();

	//RecordScene();
}

void ShadowMap::DestroyScene(bool isRecreation)
{
	graphicsPipeline.destroyGraphicsPipeline(device);
	vkDestroyPipeline(device, debugPipeline.pipeline, nullptr);
	shadowPipeline.destroyGraphicsPipeline(device);
	quadPipeline.destroyGraphicsPipeline(device);
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
	}
}

void ShadowMap::CreateUniforms(const VulkanSwapChain& swapChain)
{
	VkExtent2D dim = swapChain.swapChainDimensions;

	light = SpotLight(glm::vec3(-5.0f, 5.0f, -5.0f), glm::vec3(0.0f), glm::vec3(1.0f), 0.1f, 1000.0f);

	uboShadow.depthViewProj = light.getLightProj() * light.getLightView();

	Camera* camera = Camera::GetCamera();
	uboScene.view = camera->GetViewMatrix();
	uboScene.proj = glm::perspective(glm::radians(45.0f), float(dim.width) / float(dim.height), 0.1f, 1000.0f);
	uboScene.proj[1][1] *= -1;
	uboScene.depthBiasVP = uboShadow.depthViewProj;
	uboScene.lightPos = light.getLightPos();

	UpdateMeshes(0.0f);
}

void ShadowMap::UpdateUniforms(uint32_t index)
{
	static std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now(),
		lastTime = std::chrono::steady_clock::now();
	
	static float timer = 0.0f;
	const float twoPI = 2.0f * 3.14159f;

	float dt = 0.0f;
	currentTime = std::chrono::high_resolution_clock::now();
	dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;
	if (animate)
		timer += dt;

	UpdateMeshes(timer);

	// oscillate between -radius and +radius at speed
	float speed = 100.0f;
	float radius = 2.0f;

	// move light in a circle
	//glm::vec3 lightPos = glm::vec3(cos(glm::radians(timer * speed)), 0.0f, sin(glm::radians(timer * speed))) * radius;
	//lightPos.y = 1.0f;
	//light.setLightPos(lightPos);

	//uboShadow.depthViewProj = light.getLightProj() * light.getLightView();
	glm::vec3 lightPos = light.getLightPos();
	printf("\rLight: %2.2f, %2.2f, %2.2f", lightPos.x, lightPos.y, lightPos.z);

	static Camera* camera = Camera::GetCamera();
	uboScene.view = camera->GetViewMatrix();
	//uboScene.depthBiasVP = uboShadow.depthViewProj;
	//uboScene.lightPos = lightPos;

	memcpy(graphicsPipeline.uniformBuffers[index].mappedMemory, &uboScene, sizeof(uboScene));
	memcpy(shadowPipeline.uniformBuffers[index].mappedMemory, &uboShadow, sizeof(uboShadow));
}

void ShadowMap::UpdateMeshes(float time)
{
	glm::mat4 model = glm::scale(glm::vec3(5.0f, 0.0f, 5.0f));
	BasicShapes::getPlane()->setModelMatrix(model);

	model = glm::mat4(1.0f);
	model = glm::translate(glm::vec3(-2.0f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.75f, 0.75f, 0.75f));
	BasicShapes::getBox()->setModelMatrix(model);

	model = glm::mat4(1.0f);
	model = glm::translate(glm::vec3(2.0f, 0.5f, -2.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	BasicShapes::getSphere()->setModelMatrix(model);

	model = glm::mat4(1.0f);
	model = glm::translate(glm::vec3(0.0f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	BasicShapes::getMonkey()->setModelMatrix(model);
}

void ShadowMap::CreatePipelines(const VulkanSwapChain& swapChain)
{
#pragma region SETUP
	VkVertexInputBindingDescription bindingDescription = ModelVertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescription = ModelVertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	VkPipelineViewportStateCreateInfo viewportState = {};
	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	VkPipelineColorBlendAttachmentState colorBlendingAttachment = {};
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	// initialize pipeline info
	{
		// vertex/input assembly state
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		// viewport
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// rasterization
		rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerState.depthClampEnable = VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = VK_FALSE;
		rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerState.lineWidth = 1.0f;
		rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerState.depthBiasEnable = VK_FALSE;
		rasterizerState.depthBiasConstantFactor = 0.0f;
		rasterizerState.depthBiasClamp = 0.0f;
		rasterizerState.depthBiasSlopeFactor = 0.0f;

		// multisampling
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.minSampleShading = 1.0f;
		multisampleState.pSampleMask = nullptr;
		multisampleState.alphaToCoverageEnable = VK_FALSE;
		multisampleState.alphaToOneEnable = VK_FALSE;

		// color blending
		colorBlendingAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendingAttachment.blendEnable = VK_TRUE;
		colorBlendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendingAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendingAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendingAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendingAttachment;
		colorBlendState.blendConstants[0] = 0.0f;
		colorBlendState.blendConstants[1] = 0.0f;
		colorBlendState.blendConstants[2] = 0.0f;
		colorBlendState.blendConstants[3] = 0.0f;

		// depth/stencil testing
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.stencilTestEnable = VK_FALSE;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		graphicsPipeline.isDepthBufferEmpty = false;
		shadowPipeline.isDepthBufferEmpty = false;

		// dynamic states
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.flags = 0;
		dynamicState.pNext = nullptr;

		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState; // rasterizer
		pipelineInfo.pMultisampleState = &multisampleState; // multisampling
		pipelineInfo.pColorBlendState = &colorBlendState; // color blending
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
	}
#pragma endregion

#pragma region SCENE_PIPELINE
	{
		auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/scene_vert.spv");
		VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

		auto fragShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/scene_frag.spv");
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

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

		// models and shapes have the same descriptor set layout
		Mesh* box = BasicShapes::getBox();
		VkDescriptorSetLayout layouts[3] = { graphicsPipeline.descriptorSetLayout, box->descriptorSetLayout, box->material->descriptorSetLayout };

		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;
		layoutInfo.setLayoutCount = 3;
		layoutInfo.pSetLayouts = layouts;

		graphicsPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout);
		if (graphicsPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout");

		graphicsPipeline.viewport.x = 0.0f;
		graphicsPipeline.viewport.y = 0.0f;
		graphicsPipeline.viewport.minDepth = 0.0f;
		graphicsPipeline.viewport.maxDepth = 1.0f;
		graphicsPipeline.viewport.width = (float)swapChain.swapChainDimensions.width;
		graphicsPipeline.viewport.height = (float)swapChain.swapChainDimensions.height;
		graphicsPipeline.scissors.offset = { 0, 0 };
		graphicsPipeline.scissors.extent = swapChain.swapChainDimensions;
		viewportState.pViewports = &graphicsPipeline.viewport;
		viewportState.pScissors = &graphicsPipeline.scissors;

		VkSpecializationMapEntry debugMapEntry = {};
		debugMapEntry.constantID = 0;
		debugMapEntry.offset = 0;
		debugMapEntry.size = sizeof(int);

		VkSpecializationInfo debugInfo = {};
		debugInfo.mapEntryCount = 1;
		debugInfo.dataSize = sizeof(int);
		debugInfo.pMapEntries = &debugMapEntry;
		debugInfo.pData = &debug;

		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.renderPass = graphicsPipeline.renderPass;
		pipelineInfo.layout = graphicsPipeline.pipelineLayout;
		pipelineInfo.pViewportState = &viewportState;
		shaderStages[0].pSpecializationInfo = &debugInfo;
		shaderStages[1].pSpecializationInfo = &debugInfo;

		graphicsPipeline.result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline.pipeline);
		if (graphicsPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		debug = 1;
		debugPipeline.result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &debugPipeline.pipeline);
		if (debugPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		debug = 0;
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}
#pragma endregion

#pragma region QUAD
	{
		auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"Global/full_screen_quad.spv");
		VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

		auto fragShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/quad_frag.spv");
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

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
		
		VkPushConstantRange quadPush = {};
		quadPush.offset = 0;
		quadPush.size = 2 * sizeof(float);
		quadPush.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &quadPush;
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &quadPipeline.descriptorSetLayout;

		quadPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &quadPipeline.pipelineLayout);
		if (quadPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout");

		quadPipeline.viewport.x = 0.0f;
		quadPipeline.viewport.y = 0.0f;
		quadPipeline.viewport.minDepth = 0.0f;
		quadPipeline.viewport.maxDepth = 1.0f;
		quadPipeline.viewport.width = (float)swapChain.swapChainDimensions.width / 4.0f;
		quadPipeline.viewport.height = (float)swapChain.swapChainDimensions.height / 4.0f;
		quadPipeline.scissors.extent.width = swapChain.swapChainDimensions.width / 4;
		quadPipeline.scissors.extent.height = swapChain.swapChainDimensions.height / 4;
		quadPipeline.scissors.offset = { 0, 0 };
		viewportState.pViewports = &quadPipeline.viewport;
		viewportState.pScissors = &quadPipeline.scissors;

		VkPipelineVertexInputStateCreateInfo emptyVertexState = {};
		emptyVertexState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		emptyVertexState.vertexAttributeDescriptionCount = 0;
		emptyVertexState.pVertexAttributeDescriptions = nullptr;
		emptyVertexState.vertexBindingDescriptionCount = 0;
		emptyVertexState.pVertexBindingDescriptions = nullptr;

		rasterizerState.cullMode = VK_CULL_MODE_FRONT_BIT;

		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.renderPass = quadPipeline.renderPass;
		pipelineInfo.layout = quadPipeline.pipelineLayout;

		pipelineInfo.pVertexInputState = &emptyVertexState;

		quadPipeline.result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &quadPipeline.pipeline);
		if (quadPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

#pragma endregion

#pragma region SHADOW_PIPELINE
	{
		auto vertShaderCode = HelperFunctions::readShaderFile(SHADERPATH"ShadowMap/shadow_vert.spv");
		VkShaderModule vertShaderModule = HelperFunctions::CreateShaderModules(vertShaderCode);

		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStage.module = vertShaderModule;
		shaderStage.pName = "main";

		// no blend attachment states
		colorBlendState.attachmentCount = 0;
		colorBlendState.pAttachments = nullptr;

		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription[0]; // we only care about the vertex positions
		
		rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerState.depthBiasEnable = VK_TRUE;
		colorBlendingAttachment.blendEnable = VK_FALSE;

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
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		// ** Pipeline Layout ** 
		VkDescriptorSetLayout layouts[] = { shadowPipeline.descriptorSetLayout, BasicShapes::getBox()->descriptorSetLayout };

		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;
		layoutInfo.setLayoutCount = 2;
		layoutInfo.pSetLayouts = layouts;

		shadowPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &shadowPipeline.pipelineLayout);
		if (shadowPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout");

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.renderPass = shadowPipeline.renderPass;
		pipelineInfo.stageCount = 1;
		pipelineInfo.pStages = &shaderStage;
		pipelineInfo.renderPass = shadowPipeline.renderPass;
		pipelineInfo.layout = shadowPipeline.pipelineLayout;
		pipelineInfo.pViewportState = &viewportState;

		shadowPipeline.result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &shadowPipeline.pipeline);
		if (shadowPipeline.result != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline");

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
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
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = depthFormat;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Attachment will be transitioned to shader read at render pass end
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depthReference;

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

	// scene render pass must wait for the shadow pass to write to the depth buffer
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
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

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
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

		vkMapMemory(device, shadowPipeline.uniformBuffers[i].bufferMemory, 0, sizeof(uboShadow), 0, &shadowPipeline.uniformBuffers[i].mappedMemory);
		memcpy(shadowPipeline.uniformBuffers[i].mappedMemory, &uboShadow, sizeof(uboShadow));
		vkUnmapMemory(device, shadowPipeline.uniformBuffers[i].bufferMemory);
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
	HelperFunctions::createImage(shadowMapDim, shadowMapDim, 1, VK_IMAGE_TYPE_2D, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		shadowPipeline.depthStencilBuffer, shadowPipeline.depthStencilBufferMemory);

	HelperFunctions::createImageView(shadowPipeline.depthStencilBuffer, shadowPipeline.depthStencilBufferView, depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);

	// create sampler
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.minFilter = VK_FILTER_NEAREST;
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.anisotropyEnable = VK_FALSE;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	//sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	if (vkCreateSampler(device, &sampler, nullptr, &shadowSampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow map sampler");


	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
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

// ********* SCENE PIPELINE ***************

void ShadowMap::CreateSceneDescriptorSets(const VulkanSwapChain& swapChain)
{
#pragma region POOLS
	size_t swapChainSize = swapChain.swapChainImages.size();
	uint32_t descriptorCount = (uint32_t)swapChainSize; // 1 descriptor set per frame

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

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
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
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	allocInfo.pSetLayouts = layout.data();

	graphicsPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &allocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

#pragma endregion

#pragma region WRITE_DESCRIPTORS
	
	VkWriteDescriptorSet writeDescriptors[2] = {};

	for (size_t i = 0; i < swapChainSize; i++)
	{
		// scene descriptors
		VkDescriptorBufferInfo uboInfo = {};
		uboInfo.buffer = graphicsPipeline.uniformBuffers[i].buffer;
		uboInfo.offset = 0;
		uboInfo.range = sizeof(uboScene);

		writeDescriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptors[0].dstSet = graphicsPipeline.descriptorSets[i];
		writeDescriptors[0].dstBinding = 0;
		writeDescriptors[0].dstArrayElement = 0;
		writeDescriptors[0].descriptorCount = 1;
		writeDescriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptors[0].pBufferInfo = &uboInfo;

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		imageInfo.imageView = shadowPipeline.depthStencilBufferView;
		imageInfo.sampler = shadowSampler;

		writeDescriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptors[1].dstSet = graphicsPipeline.descriptorSets[i];
		writeDescriptors[1].dstBinding = 1;
		writeDescriptors[1].dstArrayElement = 0;
		writeDescriptors[1].descriptorCount = 1;
		writeDescriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptors[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, 2, writeDescriptors, 0, nullptr);
	}
#pragma endregion

#pragma region QUAD

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = descriptorCount;

	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &quadPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	quadPipeline.isDescriptorPoolEmpty = false;
	shadowMapBinding.binding = 0;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &shadowMapBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &quadPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");

	std::vector<VkDescriptorSetLayout> debugLayout(swapChainSize, quadPipeline.descriptorSetLayout);
	allocInfo.descriptorPool = quadPipeline.descriptorPool;
	allocInfo.pSetLayouts = debugLayout.data();

	quadPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &allocInfo, quadPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets");

	VkWriteDescriptorSet debugWrite = {};
	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		imageInfo.imageView = shadowPipeline.depthStencilBufferView;
		imageInfo.sampler = shadowSampler;

		debugWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		debugWrite.dstSet = quadPipeline.descriptorSets[i];
		debugWrite.dstBinding = 0;
		debugWrite.dstArrayElement = 0;
		debugWrite.descriptorCount = 1;
		debugWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		debugWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, 1, &debugWrite, 0, nullptr);
	}
#pragma endregion
}

void ShadowMap::CreateSceneFramebuffers(const VulkanSwapChain& swapChain)
{
	graphicsPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());
	quadPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());
	VkExtent2D dim = swapChain.swapChainDimensions;

	HelperFunctions::createImage(dim.width, dim.height, 1, VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferMemory);

	HelperFunctions::createImageView(graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferView,
		VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);

	VkImageView attachments[2];

	attachments[1] = graphicsPipeline.depthStencilBufferView;

	VkFramebufferCreateInfo sceneFramebufferInfo = {}, debugFramebufferInfo = {};
	sceneFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	sceneFramebufferInfo.renderPass = graphicsPipeline.renderPass;
	sceneFramebufferInfo.attachmentCount = 2;
	sceneFramebufferInfo.pAttachments = attachments;
	sceneFramebufferInfo.width = dim.width;
	sceneFramebufferInfo.height = dim.height;
	sceneFramebufferInfo.layers = 1;

	debugFramebufferInfo = sceneFramebufferInfo;
	debugFramebufferInfo.attachmentCount = 1;
	debugFramebufferInfo.renderPass = quadPipeline.renderPass;

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		attachments[0] = swapChain.swapChainImageViews[i];
		if (vkCreateFramebuffer(device, &sceneFramebufferInfo, nullptr, &graphicsPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");

		debugFramebufferInfo.pAttachments = &attachments[0];
		if (vkCreateFramebuffer(device, &debugFramebufferInfo, nullptr, &quadPipeline.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create one or more framebuffers");
	}
}

void ShadowMap::CreateSceneRenderPass(const VulkanSwapChain& swapChain)
{
	// scene pass

	VkAttachmentDescription attachments[2];
	VkAttachmentReference references[2];

	attachments[0].format = swapChain.swapChainImageFormat;
	attachments[0].flags = 0;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
	attachments[1].flags = 0;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	references[0].attachment = 0;
	references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	references[1].attachment = 1;
	references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &references[0];
	subpass.pDepthStencilAttachment = &references[1];

	std::array<VkSubpassDependency, 2> dependencies;

	// render pass must wait for shadow pass to finish writing to its depth buffer
	// before it can be sampled
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

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	graphicsPipeline.result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &graphicsPipeline.renderPass);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");

	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachments[0];
	subpass.pDepthStencilAttachment = nullptr;

	quadPipeline.result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &quadPipeline.renderPass);
	if (quadPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}
