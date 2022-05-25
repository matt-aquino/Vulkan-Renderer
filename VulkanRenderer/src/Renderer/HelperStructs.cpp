#include "HelperStructs.h"

// pipelines

void VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	vkMapMemory(device, bufferMemory, offset, size, 0, &mappedMemory);
	isMapped = true;
}

void VulkanBuffer::unmap()
{
	if (isMapped)
	{
		VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
		vkUnmapMemory(device, bufferMemory);
		mappedMemory = nullptr;
		isMapped = false;
	}
}

void VulkanBuffer::destroy()
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	if (buffer != VK_NULL_HANDLE)
	{
		if (isMapped)
		{
			vkUnmapMemory(device, bufferMemory);
			mappedMemory = nullptr;
		}
		
		vkDestroyBuffer(device, buffer, nullptr);
		vkFreeMemory(device, bufferMemory, nullptr);

	}
}

void VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset)
{
	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = bufferMemory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

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

VkVertexInputBindingDescription FontVertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindDesc = {};
	bindDesc.binding = 0;
	bindDesc.stride = sizeof(FontVertex);
	bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDesc;
}

std::array<VkVertexInputAttributeDescription, 3> FontVertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attrDesc{};
	attrDesc[0].binding = 0;
	attrDesc[0].location = 0; 
	attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT; 
	attrDesc[0].offset = offsetof(FontVertex, position);

	attrDesc[1].binding = 0;
	attrDesc[1].location = 1;
	attrDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDesc[1].offset = offsetof(FontVertex, texcoord);
	
	attrDesc[2].binding = 0;
	attrDesc[2].location = 2;
	attrDesc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attrDesc[2].offset = offsetof(FontVertex, color);

	return attrDesc;
}

bool FontVertex::operator==(const FontVertex& other) const
{
	return position == other.position &&
		texcoord == other.texcoord && color == other.color;
}


// textures
Texture::Texture()
{
	type = TextureType::NONE;
	image = VK_NULL_HANDLE;
	imageView = VK_NULL_HANDLE;
	imageMemory = VK_NULL_HANDLE;
	sampler = VK_NULL_HANDLE;
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
	sampler = texture->sampler; 
	textureDescriptor = texture->textureDescriptor;
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
	meshUBO.normal = glm::transpose(glm::inverse(meshUBO.model));
	memcpy(uniformBuffer.mappedMemory, &meshUBO, sizeof(meshUBO));
}

void Mesh::setMaterialColorWithValue(ColorType colorType, glm::vec3 color)
{
	switch (colorType)
	{
	case ColorType::AMBIENT:
		material->ubo.ambient = color;
		break;

	case ColorType::DIFFUSE:
		material->ubo.diffuse = color;
		break;

	case ColorType::SPECULAR:
		material->ubo.specular = color;
		break;

	case ColorType::TRANSMITTANCE:
		material->ubo.transmittance = color;
		break;

	case ColorType::EMISSION:
		material->ubo.emission = color;
		break;

	default:
		break;
	}
}

void Mesh::setMaterialWithPreset(MaterialPresets preset)
{
	switch (preset)
	{
#pragma region MINERALS
	case MaterialPresets::EMERALD:
		material->ubo.ambient = glm::vec3(0.0215f, 0.1745f, 0.0215f);
		material->ubo.diffuse = glm::vec3(0.07568f, 0.61424f, 0.07568f);
		material->ubo.specular = glm::vec3(0.633f, 0.727811f, 0.633f);
		material->ubo.shininess = 76.8f;
		material->ubo.dissolve = 0.55f;
		break;

	case MaterialPresets::RUBY:
		material->ubo.ambient = glm::vec3(0.1745f, 0.01175f, 0.01175f);
		material->ubo.diffuse = glm::vec3(0.61424f, 0.04136f, 0.04136f);
		material->ubo.specular = glm::vec3(0.727811f, 0.626959f, 0.626959f);
		material->ubo.shininess = 76.8f;
		material->ubo.dissolve = 0.55f;
		break;


	case MaterialPresets::TURQUOISE:
		material->ubo.ambient = glm::vec3(0.1f, 0.18725f, 0.1745f);
		material->ubo.diffuse = glm::vec3(0.396f, 0.74151f, 0.69102f);
		material->ubo.specular = glm::vec3(0.297254f, 0.30829f, 0.306678f);
		material->ubo.shininess = 12.8f;
		material->ubo.dissolve = 0.8f;
		break;

	case MaterialPresets::JADE:
		material->ubo.ambient = glm::vec3(0.135f, 0.2225f, 0.1575f);
		material->ubo.diffuse = glm::vec3(0.54f, 0.89f, 0.63f);
		material->ubo.specular = glm::vec3(0.316228f, 0.316228f, 0.316228f);
		material->ubo.shininess = 12.8f;
		material->ubo.dissolve = 0.95f;
		break;

	case MaterialPresets::PEARL:
		material->ubo.ambient = glm::vec3(0.25f, 0.20725f, 0.20725f);
		material->ubo.diffuse = glm::vec3(1.0f, 0.829f, 0.829f);
		material->ubo.specular = glm::vec3(0.296648f, 0.296648f, 0.296648f);
		material->ubo.shininess = 11.264f;
		material->ubo.dissolve = 0.922f;
		break;

	case MaterialPresets::OBSIDIAN:
		material->ubo.ambient = glm::vec3(0.05375f, 0.05f, 0.06625f);
		material->ubo.diffuse = glm::vec3(0.18275f, 0.17f, 0.22525f);
		material->ubo.specular = glm::vec3(0.332741f, 0.328634f, 0.346435f);
		material->ubo.shininess = 38.4f;
		material->ubo.dissolve = 0.82f;
		break;
#pragma endregion

#pragma region METALS
	case MaterialPresets::BRASS:
		material->ubo.ambient = glm::vec3(0.329412f, 0.223529f, 0.027451f);
		material->ubo.diffuse = glm::vec3(0.780392f, 0.568627f, 0.113725f);
		material->ubo.specular = glm::vec3(0.992157f, 0.941176f, 0.807843f);
		material->ubo.shininess = 27.8974f;
		break;

	case MaterialPresets::BRONZE:
		material->ubo.ambient = glm::vec3(0.2125f, 0.1275f, 0.054f);
		material->ubo.diffuse = glm::vec3(0.714f, 0.4284f, 0.18144f);
		material->ubo.specular = glm::vec3(0.393548f, 0.271906f, 0.166721f);
		material->ubo.shininess = 25.6f;
		break;

	case MaterialPresets::POLISHED_BRONZE:
		material->ubo.ambient = glm::vec3(0.25f, 0.148f, 0.06475f);
		material->ubo.diffuse = glm::vec3(0.4f, 0.2368f, 0.1036f);
		material->ubo.specular = glm::vec3(0.774597f, 0.458561f, 0.200621f);
		material->ubo.shininess = 76.8f;
		break;

	case MaterialPresets::CHROME:
		material->ubo.ambient = glm::vec3(0.25f, 0.25f, 0.25f);
		material->ubo.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
		material->ubo.specular = glm::vec3(0.774597f, 0.774597f, 0.774597f);
		material->ubo.shininess = 76.8f;
		break;

	case MaterialPresets::COPPER:
		material->ubo.ambient = glm::vec3(0.19125f, 0.0735f, 0.0225f);
		material->ubo.diffuse = glm::vec3(0.7038f, 0.27048f, 0.0828f);
		material->ubo.specular = glm::vec3(0.256777f, 0.137622f, 0.086014f);
		material->ubo.shininess = 12.8f;
		break;

	case MaterialPresets::POLISHED_COPPER:
		material->ubo.ambient = glm::vec3(0.2295f, 0.08825f, 0.0275f);
		material->ubo.diffuse = glm::vec3(0.5508f, 0.2118f, 0.066f);
		material->ubo.specular = glm::vec3(0.580594f, 0.223257f, 0.0695701f);
		material->ubo.shininess = 51.2f;
		break;

	case MaterialPresets::GOLD:
		material->ubo.ambient = glm::vec3(0.24725f, 0.1995f, 0.0745f);
		material->ubo.diffuse = glm::vec3(0.75164f, 0.60648f, 0.22648f);
		material->ubo.specular = glm::vec3(0.628281f, 0.555802f, 0.366065f);
		material->ubo.shininess = 51.2f;
		break;

	case MaterialPresets::POLISHED_GOLD:
		material->ubo.ambient = glm::vec3(0.24725f, 0.2245f, 0.0645f);
		material->ubo.diffuse = glm::vec3(0.34615f, 0.3143f, 0.0903f);
		material->ubo.specular = glm::vec3(0.797357f, 0.723991f, 0.208006f);
		material->ubo.shininess = 83.2f;
		break;

	case MaterialPresets::SILVER:
		material->ubo.ambient = glm::vec3(0.19225f, 0.19225f, 0.19225f);
		material->ubo.diffuse = glm::vec3(0.50754f, 0.50754f, 0.50754f);
		material->ubo.specular = glm::vec3(0.508273f, 0.508273f, 0.508273f);
		material->ubo.shininess = 51.2f;
		break;

	case MaterialPresets::POLISHED_SILVER:
		material->ubo.ambient = glm::vec3(0.23125f, 0.23125f, 0.23125f);
		material->ubo.diffuse = glm::vec3(0.2775f, 0.2775f, 0.2775f);
		material->ubo.specular = glm::vec3(0.773911f, 0.773911f, 0.773911f);
		material->ubo.shininess = 89.6f;
		break;
#pragma endregion

#pragma region PLASTICS
	case MaterialPresets::BLACK_PLASTIC:
		material->ubo.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.01f, 0.01f, 0.01f);
		material->ubo.specular = glm::vec3(0.50f, 0.50f, 0.50f);
		material->ubo.shininess = 32.0f;
		break;

	case MaterialPresets::WHITE_PLASTIC:
		material->ubo.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.55f, 0.55f, 0.55f);
		material->ubo.specular = glm::vec3(0.70f, 0.70f, 0.70f);
		material->ubo.shininess = 32.0f;
		break;

	case MaterialPresets::RED_PLASTIC:
		material->ubo.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.5f, 0.0f, 0.0f);
		material->ubo.specular = glm::vec3(0.7f, 0.6f, 0.6f);
		material->ubo.shininess = 32.0f;
		break;

	case MaterialPresets::YELLOW_PLASTIC:
		material->ubo.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.5f, 0.5f, 0.0f);
		material->ubo.specular = glm::vec3(0.60f, 0.60f, 0.50f);
		material->ubo.shininess = 32.0f;
		break;

	case MaterialPresets::GREEN_PLASTIC:
		material->ubo.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.1f, 0.35f, 0.1f);
		material->ubo.specular = glm::vec3(0.45f, 0.55f, 0.45f);
		material->ubo.shininess = 32.0f;
		break;

	case MaterialPresets::CYAN_PLASTIC:
		material->ubo.ambient = glm::vec3(0.0f, 0.1f, 0.06f);
		material->ubo.diffuse = glm::vec3(0.0f, 0.50980392f, 0.50980392f);
		material->ubo.specular = glm::vec3(0.50196078f, 0.50196078f, 0.50196078f);
		material->ubo.shininess = 32.0f;
		break;
#pragma endregion

#pragma region RUBBER
	case MaterialPresets::BLACK_RUBBER:
		material->ubo.ambient = glm::vec3(0.02f, 0.02f, 0.02f);
		material->ubo.diffuse = glm::vec3(0.01f, 0.01f, 0.01f);
		material->ubo.specular = glm::vec3(0.4f, 0.4f, 0.4f);
		material->ubo.shininess = 10.0f;
		break;

	case MaterialPresets::WHITE_RUBBER:
		material->ubo.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
		material->ubo.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		material->ubo.specular = glm::vec3(0.7f, 0.7f, 0.7f);
		material->ubo.shininess = 10.0f;
		break;

	case MaterialPresets::RED_RUBBER:
		material->ubo.ambient = glm::vec3(0.05f, 0.0f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.5f, 0.4f, 0.4f);
		material->ubo.specular = glm::vec3(0.7f, 0.04f, 0.04f);
		material->ubo.shininess = 10.0f;
		break;

	case MaterialPresets::YELLOW_RUBBER:
		material->ubo.ambient = glm::vec3(0.329412f, 0.223529f, 0.027451f);
		material->ubo.diffuse = glm::vec3(0.780392f, 0.568627f, 0.113725f);
		material->ubo.specular = glm::vec3(0.992157f, 0.941176f, 0.807843f);
		material->ubo.shininess = 10.0f;
		break;

	case MaterialPresets::GREEN_RUBBER:
		material->ubo.ambient = glm::vec3(0.0f, 0.05f, 0.0f);
		material->ubo.diffuse = glm::vec3(0.4f, 0.5f, 0.4f);
		material->ubo.specular = glm::vec3(0.04f, 0.7f, 0.04f);
		material->ubo.shininess = 10.0f;
		break;

	case MaterialPresets::CYAN_RUBBER:
		material->ubo.ambient = glm::vec3(0.0f, 0.05f, 0.05f);
		material->ubo.diffuse = glm::vec3(0.4f, 0.5f, 0.5f);
		material->ubo.specular = glm::vec3(0.04f, 0.7f, 0.7f);
		material->ubo.shininess = 10.0f;
		break;
#pragma endregion

	default:
		break;
	}

	memcpy(material->uniformBuffer.mappedMemory, &material->ubo, sizeof(material->ubo));
}

void Mesh::setMaterialValue(MaterialValueType valueType, float value)
{
	switch (valueType)
	{
	case MaterialValueType::SHININESS:
		material->ubo.shininess = value;
		break;

	case MaterialValueType::REFRACTION:
		material->ubo.ior = value;
		break;

	case MaterialValueType::DISSOLVE:
		material->ubo.dissolve = value;
		break;

	case MaterialValueType::ROUGHNESS:
		material->ubo.roughness = value;
		break;

	case MaterialValueType::METALLIC:
		material->ubo.metallic = value;
		break;

	case MaterialValueType::SHEEN:
		material->ubo.sheen = value;
		break;

	case MaterialValueType::CC_THICKNESS:
		material->ubo.clearcoat_thickness = value;
		break;

	case MaterialValueType::CC_ROUGHNESS:
		material->ubo.clearcoat_roughness = value;
		break;

	case MaterialValueType::ANISOTROPY:
		material->ubo.anisotropy = value;
		break;

	case MaterialValueType::ANISOTROPY_ROT:
		material->ubo.anisotropy_rotation = value;
		break;

	default:
		break;
	}
}

void Mesh::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	// bind model uniform buffer
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
		1, 1, &descriptorSet, 0, nullptr);

	if (useMaterial)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			2, 1, &material->descriptorSet, 0, nullptr);
	}

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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

void Model::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial)
{
	for (size_t i = 0; i < meshes.size(); i++)
	{
		meshes[i]->draw(commandBuffer, pipelineLayout, useMaterial);
	}
}

