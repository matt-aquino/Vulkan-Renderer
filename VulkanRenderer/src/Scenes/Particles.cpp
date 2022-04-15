#include "Particles.h"
#include <random>

const int MAX_NUM_PARTICLES = 1024 * 1024;
const int WORK_GROUP_SIZE = 256;
const int NUM_GROUPS = MAX_NUM_PARTICLES / WORK_GROUP_SIZE;

Particles::Particles(std::string name, const VulkanSwapChain& swapChain)
{
	sceneName = name;

	srand(unsigned int (time(NULL))); // seed random number generator

	CreateCommandPool();
	CreateUniforms(swapChain);
	CreateParticles(false);
	CreateRenderPass(swapChain);
	CreateFramebuffers(swapChain);
	CreateCommandBuffers();
	CreateSyncObjects(swapChain);
	CreateComputeDescriptorSets(swapChain);
	CreateComputePipeline();
	CreateGraphicsDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);

	RecordScene();

}

Particles::~Particles()
{
}

void Particles::RecordScene()
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = graphicsPipeline.scissors.extent;
	renderPassInfo.renderPass = graphicsPipeline.renderPass;

	VkClearValue clearColors[2] = {};
	clearColors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[1].depthStencil = { 1.0f, 0 };

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkDeviceSize offsets[] = { 0 };

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // allows resubmission while also already pending execution.

	// begin recording command buffers
	for (size_t i = 0; i < commandBuffersList.size(); i++)
	{
		if (vkBeginCommandBuffer(commandBuffersList[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Failed to being recording command buffer!");

		renderPassInfo.framebuffer = graphicsPipeline.framebuffers[i];
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColors;

		// bind compute pipeline and dispatch compute shader to calculate particle positions
		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 
			0, 1, &computePipeline.descriptorSet, 0, nullptr);
		vkCmdPushConstants(commandBuffersList[i], computePipeline.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePush), &pushConstants);
		vkCmdDispatch(commandBuffersList[i], NUM_GROUPS, 1, 1);

		// bind graphics pipeline and begin render pass to draw particles to framebuffers
		vkCmdBeginRenderPass(commandBuffersList[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffersList[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
			0, 1, &graphicsPipeline.descriptorSets[i], 0, nullptr);

		// bind storage buffer as a vertex buffer
		vkCmdBindVertexBuffers(commandBuffersList[i], 0, 1, &computePipeline.storageBuffer->buffer, offsets);

		// draw particles
		vkCmdDraw(commandBuffersList[i], MAX_NUM_PARTICLES, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffersList[i]);

		if (vkEndCommandBuffer(commandBuffersList[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to record command buffer");
	}
}

void Particles::RecreateScene(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	vkDeviceWaitIdle(device); // wait for all operations to finish
	ReadBackParticleData();

	DestroyScene(true);

	CreateUniforms(swapChain);
	CreateParticles(true);
	CreateRenderPass(swapChain);
	CreateFramebuffers(swapChain);
	CreateCommandBuffers();
	CreateComputeDescriptorSets(swapChain);
	CreateComputePipeline();
	CreateGraphicsDescriptorSets(swapChain);
	CreateGraphicsPipeline(swapChain);

	RecordScene();
}

VulkanReturnValues Particles::DrawScene(const VulkanSwapChain& swapChain)
{

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	/* Scene Main Loop
	*  1. Acquire image from swap chain
	*  2. Select appropriate command buffer, execute it
	*  3. Return image to swap chain for presentation
	*/

	uint32_t imageIndex;
	graphicsPipeline.result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}


	UpdateUniforms(currentFrame);

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

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
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

	graphicsPipeline.result = vkQueuePresentKHR(VulkanDevice::GetVulkanDevice()->GetQueues().presentQueue, &presentInfo);

	if (graphicsPipeline.result == VK_ERROR_OUT_OF_DATE_KHR || graphicsPipeline.result == VK_SUBOPTIMAL_KHR)
		return VulkanReturnValues::VK_SWAPCHAIN_OUT_OF_DATE;

	else if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to present swap chain image");

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return VulkanReturnValues::VK_FUNCTION_SUCCESS;
}

void Particles::DestroyScene(bool isRecreation)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	
	graphicsPipeline.destroyGraphicsPipeline(device);

	size_t size = commandBuffersList.size();
	vkFreeCommandBuffers(device, commandPool, (uint32_t)size, commandBuffersList.data());

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
		
	computePipeline.destroyComputePipeline(device);
}

void Particles::HandleKeyboardInput(const uint8_t* keystates, float dt)
{
	Camera* camera = Camera::GetCamera();

	// camera movement
	if (keystates[SDL_SCANCODE_A])
		camera->HandleInput(KeyboardInputs::LEFT, dt);

	else if (keystates[SDL_SCANCODE_D])
		camera->HandleInput(KeyboardInputs::RIGHT, dt);

	if (keystates[SDL_SCANCODE_S])
		camera->HandleInput(KeyboardInputs::FORWARD, dt);

	else if (keystates[SDL_SCANCODE_W])
		camera->HandleInput(KeyboardInputs::BACKWARD, dt);

	if (keystates[SDL_SCANCODE_Q])
		camera->HandleInput(KeyboardInputs::DOWN, dt);

	else if (keystates[SDL_SCANCODE_E])
		camera->HandleInput(KeyboardInputs::UP, dt);
}

void Particles::HandleMouseInput(const int x, const int y)
{
	static float deltaX = 0.0f;
	static float deltaY = 0.0f;

	// check if current motion is less than/greater than last motion
	float sensivity = 0.1f;

	deltaX = x * sensivity;
	deltaY = y * sensivity;

	Camera::GetCamera()->RotateCamera(deltaX, deltaY);
}

void Particles::CreateCommandBuffers()
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

void Particles::CreateParticles(bool isRecreation)
{
	pushConstants.xBounds = (std::rand() % 4) + 1; 
	pushConstants.yBounds = (std::rand() % 4) + 1;
	pushConstants.zBounds = (std::rand() % 4) + 1; // add 1 to make sure we always have a bound

	// don't recreate all the particles from scratch
	// simply recreate the buffer and reinput the data
	if (!isRecreation)
	{
		// randomly fill particle data
		for (int i = 0; i < MAX_NUM_PARTICLES; i++)
		{
			// random number between -1.0 and 1.0
			glm::vec3 position = {
				float((std::rand() % 100 - 50) / 50.0f),
				float((std::rand() % 100 - 50) / 50.0f),
				float((std::rand() % 100 - 50) / 50.0f),
			};

			glm::vec3 velocity = {
				std::rand() % 5,
				std::rand() % 5,
				std::rand() % 5,
			};

			// grab random RGB ranged from 0.1f - 1.0f; avoids all black particles
			glm::vec3 color = {
				(float)((std::rand() % 255 + 0.1f) / 255.0f),
				(float)((std::rand() % 255 + 0.1f) / 255.0f),
				(float)((std::rand() % 255 + 0.1f) / 255.0f)
			};

			Particle newParticle = { position, velocity, color };
			particles.push_back(newParticle);
		}

	}
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkDeviceSize size = sizeof(particles[0]) * particles.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory);

	// map particle data to storage buffer
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, particles.data(), (size_t)size);
	vkUnmapMemory(device, stagingBufferMemory);
	
	VulkanBuffer buffer;

	// since we'll be reading this data back, we need to make sure the CPU can see it 
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		buffer.buffer, buffer.bufferMemory);

	copyBuffer(stagingBuffer, buffer.buffer, size, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	computePipeline.storageBuffer = buffer;

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Particles::CreateUniforms(const VulkanSwapChain& swapChain)
{
	Camera* const camera = Camera::GetCamera();
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	ubo.model = glm::mat4(1.0f);
	ubo.view = camera->GetViewMatrix();
	ubo.proj = glm::perspective(glm::radians(camera->GetFOV()), float(swapChain.swapChainDimensions.width / swapChain.swapChainDimensions.height), 0.1f, 1000.0f);
	ubo.proj[1][1] *= -1;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkDeviceSize size = sizeof(UBO);

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, &ubo, size);
	vkUnmapMemory(device, stagingBufferMemory);

	size_t swapChainSize = swapChain.swapChainImages.size();
	graphicsPipeline.uniformBuffers.resize(swapChainSize);

	// create graphics uniform buffers
	for (size_t i = 0; i < swapChainSize; i++)
	{
		VulkanBuffer ub;

		createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ub.buffer, ub.bufferMemory);

		copyBuffer(stagingBuffer, ub.buffer, size, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);
		graphicsPipeline.uniformBuffers[i] = ub;
	}

	VulkanBuffer buffer;

	// create compute uniform buffer
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		buffer.buffer, buffer.bufferMemory);

	copyBuffer(stagingBuffer, buffer.buffer, size, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	computePipeline.uniformBuffer = buffer;

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Particles::UpdateUniforms(uint32_t currentFrame)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	Camera* const camera = Camera::GetCamera();
	ubo.view = camera->GetViewMatrix();

	void* data;
	vkMapMemory(device, graphicsPipeline.uniformBuffers[currentFrame].bufferMemory, 0, sizeof(UBO), 0, &data);
	memcpy(data, &ubo, sizeof(UBO));
	vkUnmapMemory(device, graphicsPipeline.uniformBuffers[currentFrame].bufferMemory);
}

void Particles::CreateRenderPass(const VulkanSwapChain& swapChain)
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

	// vertex shader depends on compute shader to finish calculating particle data for next frame
	// documentation says this is a better way of synchronization compared to pipeline barriers
	// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#compute-to-graphics-dependencies

	// vertex shader waits on compute shader
	VkSubpassDependency computeFinishedDependency{};
	computeFinishedDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	computeFinishedDependency.dstSubpass = 0;
	computeFinishedDependency.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	computeFinishedDependency.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	computeFinishedDependency.dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	computeFinishedDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	// compute shader waits on vertex shader
	VkSubpassDependency vertexFinishedDependency{};
	vertexFinishedDependency.srcSubpass = 0;
	vertexFinishedDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	vertexFinishedDependency.srcStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	vertexFinishedDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vertexFinishedDependency.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	vertexFinishedDependency.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkSubpassDependency dependencies[] = { computeFinishedDependency, vertexFinishedDependency , dependency };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 3;
	renderPassInfo.pDependencies = dependencies;

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	graphicsPipeline.result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &graphicsPipeline.renderPass);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass");
}

void Particles::CreateFramebuffers(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	graphicsPipeline.framebuffers.resize(swapChain.swapChainImageViews.size());
	VkExtent2D dim = swapChain.swapChainDimensions;

	createImage(dim.width, dim.height, 1, VK_IMAGE_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferMemory);

	createImageView(graphicsPipeline.depthStencilBuffer, graphicsPipeline.depthStencilBufferView, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);

	for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { swapChain.swapChainImageViews[i], graphicsPipeline.depthStencilBufferView };

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

void Particles::CreateGraphicsPipeline(const VulkanSwapChain& swapChain)
{
#pragma region SHADERS
	auto vertShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Particles/particles_vertex_shader.spv");
	VkShaderModule vertShaderModule = CreateShaderModules(vertShaderCode);
	
	auto fragShaderCode = graphicsPipeline.readShaderFile(SHADERPATH"Particles/particles_fragment_shader.spv");
	VkShaderModule fragShaderModule = CreateShaderModules(fragShaderCode);


	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";


#pragma endregion

#pragma region VERTEX_INPUT_STATE
	VkVertexInputBindingDescription bindDesc{};
	bindDesc.binding = 0;
	bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindDesc.stride = sizeof(Particle);

	VkVertexInputAttributeDescription attrDesc[2] = {};
	attrDesc[0].binding = 0;
	attrDesc[0].location = 0;
	attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDesc[0].offset = offsetof(Particle, Particle::position);

	attrDesc[1].binding = 0;
	attrDesc[1].location = 1;
	attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDesc[1].offset = offsetof(Particle, Particle::color);


	// ** Vertex Input State **
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
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

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkPipelineShaderStageCreateInfo stages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// ** Pipeline Layout ** 
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &graphicsPipeline.descriptorSetLayout;

	graphicsPipeline.result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &graphicsPipeline.pipelineLayout);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout");


	// ** Graphics Pipeline **
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = 2;
	graphicsPipelineInfo.pStages = stages;
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pViewportState = &viewportState;
	graphicsPipelineInfo.pRasterizationState = &rasterizerInfo; 
	graphicsPipelineInfo.pMultisampleState = &multisamplingInfo;
	graphicsPipelineInfo.pColorBlendState = &colorBlendingInfo; 
	graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
	graphicsPipelineInfo.layout = graphicsPipeline.pipelineLayout;
	graphicsPipelineInfo.renderPass = graphicsPipeline.renderPass;
	graphicsPipelineInfo.subpass = 0;
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineInfo.basePipelineIndex = -1;

	
	graphicsPipeline.result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline.pipeline);
	if (graphicsPipeline.result != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline");

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void Particles::CreateComputePipeline()
{
	auto compShaderCode = computePipeline.readShaderFile(SHADERPATH"Particles/particles_compute_shader.spv");
	VkShaderModule compShaderModule = CreateShaderModules(compShaderCode);

	// Specialization Constants
	int data[] = { MAX_NUM_PARTICLES };

	VkSpecializationMapEntry entries =
	{
		0,
		0,
		sizeof(int)
	};

	VkSpecializationInfo specInfo;
	specInfo.dataSize = sizeof(int);
	specInfo.mapEntryCount = 1;
	specInfo.pMapEntries = &entries;
	specInfo.pData = data;

	VkPushConstantRange push{};
	push.size = sizeof(ComputePush);
	push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	push.offset = 0;

	VkPipelineShaderStageCreateInfo compShaderStageInfo{};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main";
	compShaderStageInfo.pSpecializationInfo = &specInfo;

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	VkPipelineLayoutCreateInfo computeLayoutInfo{};
	computeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayoutInfo.setLayoutCount = 1;
	computeLayoutInfo.pSetLayouts = &computePipeline.descriptorSetLayout;
	computeLayoutInfo.pushConstantRangeCount = 1;
	computeLayoutInfo.pPushConstantRanges = &push;

	if (vkCreatePipelineLayout(device, &computeLayoutInfo, nullptr, &computePipeline.pipelineLayout))
		throw std::runtime_error("Failed to create compute pipeline layout!");

	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage = compShaderStageInfo;
	computePipelineInfo.layout = computePipeline.pipelineLayout;
	computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	computePipelineInfo.basePipelineIndex = -1;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline.pipeline))
		throw std::runtime_error("Failed to create compute pipeline");

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}

void Particles::CreateSyncObjects(const VulkanSwapChain& swapChain)
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

	computeFinishedBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	computeFinishedBarrier.pNext = nullptr;
	computeFinishedBarrier.buffer = computePipeline.storageBuffer->buffer;
	computeFinishedBarrier.size = sizeof(Particle) * MAX_NUM_PARTICLES;
	computeFinishedBarrier.offset = 0;
	computeFinishedBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeFinishedBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeFinishedBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	computeFinishedBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

	vertexFinishedBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	vertexFinishedBarrier.pNext = nullptr;
	vertexFinishedBarrier.buffer = computePipeline.storageBuffer->buffer;
	vertexFinishedBarrier.size = sizeof(Particle) * MAX_NUM_PARTICLES;
	vertexFinishedBarrier.offset = 0;
	vertexFinishedBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	vertexFinishedBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	vertexFinishedBarrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	vertexFinishedBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
}

void Particles::CreateGraphicsDescriptorSets(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// create descriptor sets for UBO
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics descriptor set layout");

	
	size_t swapChainSize = swapChain.swapChainImages.size();

	// create descriptor pool for graphics pipeline

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(swapChainSize);

	VkDescriptorPoolCreateInfo graphicsPoolInfo{};
	graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	graphicsPoolInfo.poolSizeCount = 1;
	graphicsPoolInfo.pPoolSizes = &poolSize;
	graphicsPoolInfo.maxSets = static_cast<uint32_t>(swapChainSize);

	if (vkCreateDescriptorPool(device, &graphicsPoolInfo, nullptr, &graphicsPipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics descriptor pool");
	graphicsPipeline.isDescriptorPoolEmpty = false;

	std::vector<VkDescriptorSetLayout> layout(swapChainSize, graphicsPipeline.descriptorSetLayout);

	// create descriptor sets
	VkDescriptorSetAllocateInfo graphicsAllocInfo{};
	graphicsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	graphicsAllocInfo.descriptorPool = graphicsPipeline.descriptorPool;
	graphicsAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);
	graphicsAllocInfo.pSetLayouts = layout.data();

	graphicsPipeline.descriptorSets.resize(swapChainSize);

	if (vkAllocateDescriptorSets(device, &graphicsAllocInfo, graphicsPipeline.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate graphics descriptor sets");

	// write descriptors
	for (size_t i = 0; i < swapChainSize; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer =	graphicsPipeline.uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBO);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = graphicsPipeline.descriptorSets[i];
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}

void Particles::CreateComputeDescriptorSets(const VulkanSwapChain& swapChain)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// create storage buffer binding for particles
	VkDescriptorSetLayoutBinding ssboBinding = {};
	ssboBinding.binding = 0;
	ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ssboBinding.descriptorCount = 1;
	ssboBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	// create binding for uniform buffer
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 1;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding bindings[] = { ssboBinding, uboBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &computePipeline.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create compute descriptor set layout!");

	// create descriptor pools

	VkDescriptorPoolSize ssboPoolSize{};
	ssboPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ssboPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize uboPoolSize{};
	uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize sizes[] = { ssboPoolSize, uboPoolSize };
	VkDescriptorPoolCreateInfo computePoolInfo{};
	computePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	computePoolInfo.poolSizeCount = 2;
	computePoolInfo.pPoolSizes = sizes;
	computePoolInfo.maxSets = 2;

	if (vkCreateDescriptorPool(device, &computePoolInfo, nullptr, &computePipeline.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create compute descriptor pool");

	// create descriptor sets
	VkDescriptorSetAllocateInfo computeAllocInfo{};
	computeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	computeAllocInfo.descriptorPool = computePipeline.descriptorPool;
	computeAllocInfo.descriptorSetCount = 1;
	computeAllocInfo.pSetLayouts = &computePipeline.descriptorSetLayout;

	if (vkAllocateDescriptorSets(device, &computeAllocInfo, &computePipeline.descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocated compute descriptor sets");

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = computePipeline.storageBuffer->buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(Particle) * MAX_NUM_PARTICLES;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = computePipeline.descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrite.pBufferInfo = &bufferInfo;

	VkDescriptorBufferInfo uboInfo = {};
	uboInfo.buffer = computePipeline.uniformBuffer->buffer;
	uboInfo.offset = 0;
	uboInfo.range = sizeof(UBO);

	VkWriteDescriptorSet uboWrite = {};
	uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uboWrite.dstSet = computePipeline.descriptorSet;
	uboWrite.dstBinding = 1;
	uboWrite.dstArrayElement = 0;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.pBufferInfo = &uboInfo;

	VkWriteDescriptorSet writes[] = { descriptorWrite, uboWrite };

	vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
}

void Particles::ReadBackParticleData()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	// read back particle data
	VkDeviceSize size = sizeof(particles[0]) * MAX_NUM_PARTICLES;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	copyBuffer(computePipeline.storageBuffer->buffer, stagingBuffer, size, VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(particles.data(), data, (size_t)size);
	vkUnmapMemory(device, stagingBufferMemory);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

}