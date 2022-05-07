#include "HelperStructs.h"

// pipelines

void VulkanGraphicsPipeline::destroyGraphicsPipeline(const VkDevice& device)
{
	for (VkFramebuffer fb : framebuffers)
	{
		vkDestroyFramebuffer(device, fb, nullptr);
	}

	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{
		vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
		vkFreeMemory(device, uniformBuffers[i].bufferMemory, nullptr);
	}

	// check is buffers are filled before attempting to delete them
	if (vertexBuffer.has_value())
	{
		vkDestroyBuffer(device, vertexBuffer->buffer, nullptr);
		vkFreeMemory(device, vertexBuffer->bufferMemory, nullptr);
	}

	if (indexBuffer.has_value())
	{
		vkDestroyBuffer(device, indexBuffer->buffer, nullptr);
		vkFreeMemory(device, indexBuffer->bufferMemory, nullptr);
	}

	if (!isDepthBufferEmpty)
	{
		vkDestroyImageView(device, depthStencilBufferView, nullptr);
		vkDestroyImage(device, depthStencilBuffer, nullptr);
		vkFreeMemory(device, depthStencilBufferMemory, nullptr);
	}

	if (!isDescriptorPoolEmpty)
	{
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	}

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
}

void VulkanComputePipeline::destroyComputePipeline(const VkDevice& device)
{
	if (storageBuffer.has_value())
	{
		vkDestroyBuffer(device, storageBuffer->buffer, nullptr);
		vkFreeMemory(device, storageBuffer->bufferMemory, nullptr);
	}

	if (uniformBuffer.has_value())
	{
		vkDestroyBuffer(device, uniformBuffer->buffer, nullptr);
		vkFreeMemory(device, uniformBuffer->bufferMemory, nullptr);
	}

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}


// vertices
VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindDesc = {};
	bindDesc.binding = 0;
	bindDesc.stride = sizeof(Vertex);
	bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDesc;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> attrDesc{};
	attrDesc[0].binding = 0;
	attrDesc[0].location = 0; // match the location within the shader
	attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; // match format within shader (float, vec2, vec3, vec4,)
	attrDesc[0].offset = offsetof(Vertex, position);

	attrDesc[1].binding = 0;
	attrDesc[1].location = 1;
	attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDesc[1].offset = offsetof(Vertex, color);

	return attrDesc;
}

VkVertexInputBindingDescription ModelVertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindDesc = {};
	bindDesc.binding = 0;
	bindDesc.stride = sizeof(ModelVertex);
	bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDesc;
}

std::array<VkVertexInputAttributeDescription, 3> ModelVertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attrDesc{};
	attrDesc[0].binding = 0;
	attrDesc[0].location = 0; // match the location within the shader
	attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; // match format within shader (float, vec2, vec3, vec4,)
	attrDesc[0].offset = offsetof(ModelVertex, position);

	attrDesc[1].binding = 0;
	attrDesc[1].location = 1;
	attrDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDesc[1].offset = offsetof(ModelVertex, texcoord);
	
	attrDesc[2].binding = 0;
	attrDesc[2].location = 2;
	attrDesc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDesc[2].offset = offsetof(ModelVertex, normal);

	return attrDesc;
}

bool ModelVertex::operator==(const ModelVertex& other) const
{
	return position == other.position &&
		texcoord == other.texcoord && normal == other.normal;
}


// textures
Texture::Texture()
{
	type = TextureType::NONE;
	image = VK_NULL_HANDLE;
	imageView = VK_NULL_HANDLE;
	imageMemory = VK_NULL_HANDLE;
	HelperFunctions::createSampler(sampler, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_TRUE, 4.0f, 0.0f, 1.0f);
	textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureDescriptor.imageView = VK_NULL_HANDLE;
	textureDescriptor.sampler = sampler;
}

Texture::Texture(TextureType textureType)
{
	type = textureType;
	image = VK_NULL_HANDLE;
	imageView = VK_NULL_HANDLE;
	imageMemory = VK_NULL_HANDLE;
	sampler = VK_NULL_HANDLE;
	textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureDescriptor.imageView = VK_NULL_HANDLE;
	textureDescriptor.sampler = sampler;
}

bool Texture::operator==(const Texture* texture)
{
	return (image == texture->image &&
		imageView == texture->imageView &&
		imageMemory == texture->imageMemory &&
		sampler == texture->sampler);
}

bool Texture::operator!=(const Texture* texture)
{
	return (image != texture->image &&
		imageView != texture->imageView &&
		imageMemory != texture->imageMemory &&
		sampler != texture->sampler);
}

void Texture::copyImageData(const Texture* texture)
{
	image = texture->image;
	imageView = texture->imageView;
	imageMemory = texture->imageMemory;
	textureDescriptor = texture->textureDescriptor;
	sampler = texture->sampler;
	width = texture->width;
	height = texture->height;
	mipLevels = texture->mipLevels;
	layerCount = texture->layerCount;
}

void Texture::destroyTexture()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	if (image != VK_NULL_HANDLE)
	{
		vkDestroyImage(device, image, nullptr);
		vkDestroyImageView(device, imageView, nullptr);
		vkDestroySampler(device, sampler, nullptr);
		vkFreeMemory(device, imageMemory, nullptr);
	}
}

// material
Material::Material()
{
	ambientTex = new Texture(TextureType::AMBIENT);
	diffuseTex = new Texture(TextureType::DIFFUSE);
	specularTex = new Texture(TextureType::SPECULAR);
	specularHighlightTex = new Texture(TextureType::SPECULAR_HIGHLIGHT);
	normalTex = new Texture(TextureType::NORMAL);
	alphaTex = new Texture(TextureType::ALPHA);
	metallicTex = new Texture(TextureType::METALLIC);
	displacementTex = new Texture(TextureType::DISPLACEMENT);
	emissiveTex = new Texture(TextureType::EMISSIVE);
	reflectionTex = new Texture(TextureType::REFLECTION);
	roughnessTex = new Texture(TextureType::ROUGHNESS);
	sheenTex = new Texture(TextureType::SHEEN);

	ubo.ambient = glm::vec3(0.15f);
	ubo.diffuse = glm::vec3(1.0f);
}

void Material::destroy()
{
	delete ambientTex;
	delete diffuseTex;
	delete specularTex;
	delete specularHighlightTex;
	delete normalTex;
	delete alphaTex;
	delete metallicTex;
	delete displacementTex;
	delete emissiveTex;
	delete reflectionTex;
	delete roughnessTex;
	delete sheenTex;

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	vkDestroyBuffer(device, uniformBuffer.buffer, nullptr);
	vkFreeMemory(device, uniformBuffer.bufferMemory, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void Material::createDescriptorSet(Texture* emptyTexture)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	std::vector<Texture*> textures = {
		ambientTex,
		diffuseTex,
		specularTex,
		specularHighlightTex,
		normalTex,
		alphaTex,
		metallicTex,
		displacementTex,
		emissiveTex,
		reflectionTex,
		roughnessTex,
		sheenTex
	};
	size_t numTextures = textures.size();

	VkDescriptorPoolSize poolSizes[2] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(textures.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	VkDescriptorSetLayoutBinding textureBinding = {}, dataBinding = {};
	dataBinding.binding = 0;
	dataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	dataBinding.descriptorCount = 1;
	dataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	dataBinding.pImmutableSamplers = nullptr;
	bindings.push_back(dataBinding);

	// combined image samplers for textures
	for (size_t i = 0; i < numTextures; i++)
	{
		VkDescriptorSetLayoutBinding textureBinding = {};
		textureBinding.binding = (uint32_t)textures[i]->type;
		textureBinding.descriptorCount = 1;
		textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureBinding.pImmutableSamplers = nullptr;
		textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings.push_back(textureBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");

	VkDeviceSize bufferSize = sizeof(ubo);
	HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer.buffer, uniformBuffer.bufferMemory);

	vkMapMemory(device, uniformBuffer.bufferMemory, 0, bufferSize, 0, &uniformBuffer.mappedMemory);
	memcpy(uniformBuffer.mappedMemory, &ubo, size_t(bufferSize));
	vkUnmapMemory(device, uniformBuffer.bufferMemory);


	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Failed to create material descriptor set");

	// fill image descriptors and create write descriptors
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(ubo);

	std::vector<VkWriteDescriptorSet> writes;

	VkWriteDescriptorSet uboWrite = {};
	uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uboWrite.dstSet = descriptorSet;
	uboWrite.dstBinding = 0;
	uboWrite.dstArrayElement = 0;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.pBufferInfo = &bufferInfo;
	writes.push_back(uboWrite);

	for (size_t i = 0; i < numTextures; i++)
	{
		VkWriteDescriptorSet textureWrite = {};
		textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.descriptorCount = 1;
		textureWrite.dstSet = descriptorSet;
		textureWrite.dstBinding = (uint32_t)textures[i]->type;
		if (textures[i]->image != VK_NULL_HANDLE)
			textureWrite.pImageInfo = &textures[i]->textureDescriptor;
		else
			textureWrite.pImageInfo = &emptyTexture->textureDescriptor;

		writes.push_back(textureWrite);
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Mesh::createDescriptorSet()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool");

	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding.pImmutableSamplers = nullptr;
		

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout!");

	VkDeviceSize bufferSize = sizeof(meshUBO);
	HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer.buffer, uniformBuffer.bufferMemory);

	vkMapMemory(device, uniformBuffer.bufferMemory, 0, bufferSize, 0, &uniformBuffer.mappedMemory);
	memcpy(uniformBuffer.mappedMemory, &meshUBO, size_t(bufferSize));
	vkUnmapMemory(device, uniformBuffer.bufferMemory);


	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Failed to create material descriptor set");

	// fill image descriptors and create write descriptors
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(meshUBO);

	VkWriteDescriptorSet uboWrite = {};
	uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uboWrite.dstSet = descriptorSet;
	uboWrite.dstBinding = 0;
	uboWrite.dstArrayElement = 0;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, 1, &uboWrite, 0, nullptr);
}

void Mesh::setModelMatrix(glm::mat4 m)
{
	meshUBO.model = m;

	memcpy(uniformBuffer.mappedMemory, &meshUBO, sizeof(meshUBO));
}

// mesh
void Mesh::destroyMesh()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	material->destroy();

	vkDestroyBuffer(device, vertexBuffer.buffer, nullptr);
	vkDestroyBuffer(device, indexBuffer.buffer, nullptr);
	vkDestroyBuffer(device, uniformBuffer.buffer, nullptr);

	vkFreeMemory(device, vertexBuffer.bufferMemory, nullptr);
	vkFreeMemory(device, indexBuffer.bufferMemory, nullptr);
	vkFreeMemory(device, uniformBuffer.bufferMemory, nullptr);

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

// model
void Model::destroyModel()
{
	while (!meshes.empty())
	{
		meshes[0]->destroyMesh();
		delete meshes[0];
		meshes.erase(meshes.begin());
	}
}

void Model::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useTextures)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkDeviceSize offsets[] = { 0 };

	for (size_t i = 0; i < meshes.size(); i++)
	{
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshes[i]->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, meshes[i]->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		
		// bind model uniform buffer
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &meshes[i]->descriptorSet, 0, nullptr);

		if (useTextures)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &meshes[i]->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshes[i]->indices.size()), 1, 0, 0, 0);
	}
}

