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

#include "Loaders.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace shared
{
	static VkCommandPool commandPool;
}

namespace TextureLoader
{
	namespace priv
	{
		static Texture* emptyTexture = nullptr;
		static std::unordered_map<std::string, Texture*> loadedTextures;
	}
}

void ModelLoader::destroy()
{

}

Model* ModelLoader::loadModel(std::string folder, std::string file)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> mats;
	std::string warn, err;
	std::vector<std::string> textureNames;

	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	static Texture* emptyTexture = TextureLoader::getEmptyTexture();
	
	std::string filePath = folder + "/" + file;

	// load in vertex and material data
	if (tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err, (directory + filePath).c_str(), (directory + folder).c_str()))
	{
		// load in materials
		for (tinyobj::material_t mat : mats)
		{
			Material* material = new Material();
			material->name = mat.name;

			// read in material data
			{
				material->ubo.ambient = {
					mat.ambient[0],
					mat.ambient[1],
					mat.ambient[2],
				};

				material->ubo.diffuse = {
					mat.diffuse[0],
					mat.diffuse[1],
					mat.diffuse[2]
				};

				material->ubo.specular = {
					mat.specular[0],
					mat.specular[1],
					mat.specular[2]
				};

				material->ubo.transmittance = {
					mat.transmittance[0],
					mat.transmittance[1],
					mat.transmittance[2]
				};

				material->ubo.emission = {
					mat.emission[0],
					mat.emission[1],
					mat.emission[2]
				};

				material->ubo.shininess = mat.shininess;
				material->ubo.ior = mat.ior;
				material->ubo.dissolve = mat.dissolve;
				material->ubo.illum = mat.illum;
				material->ubo.roughness = mat.roughness;
				material->ubo.metallic = mat.metallic;
				material->ubo.sheen = mat.sheen;
				material->ubo.clearcoat_thickness = mat.clearcoat_thickness;
				material->ubo.clearcoat_roughness = mat.clearcoat_roughness;
				material->ubo.anisotropy = mat.anisotropy;
				material->ubo.anisotropy_rotation = mat.anisotropy_rotation;
				material->ubo.pad0 = mat.pad0;
				material->ubo.dummy = mat.dummy;
			}

			// load textures
			{
				// meshes may not have certain textures bound, so we make sure they're all empty black textures
				// so that our descriptors remain valid

				// if we've already created a certain texture before, but need to use it in a different binding
				// e.g we use a diffuse map as an alpha map
				// then we simply use that texture's image, image view, and sampler, but change the binding
				// otherwise, create a brand new texture
				if (mat.ambient_texname != "")
				{
					material->ambientTex->copyImageData(TextureLoader::loadTexture(folder, mat.ambient_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.diffuse_texname != "")
				{
					material->diffuseTex->copyImageData(TextureLoader::loadTexture(folder, mat.diffuse_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}
				
				if (mat.specular_texname != "")
				{
					material->specularTex->copyImageData(TextureLoader::loadTexture(folder, mat.specular_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.specular_highlight_texname != "")
				{
					material->specularHighlightTex->copyImageData(TextureLoader::loadTexture(folder, mat.specular_highlight_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.normal_texname != "")
				{
					material->normalTex->copyImageData(TextureLoader::loadTexture(folder, mat.normal_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.roughness_texname != "")
				{
					material->roughnessTex->copyImageData(TextureLoader::loadTexture(folder, mat.roughness_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.metallic_texname != "")
				{
					material->metallicTex->copyImageData(TextureLoader::loadTexture(folder, mat.metallic_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.displacement_texname != "")
				{
					material->displacementTex->copyImageData(TextureLoader::loadTexture(folder, mat.displacement_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.emissive_texname != "")
				{
					material->emissiveTex->copyImageData(TextureLoader::loadTexture(folder, mat.emissive_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.reflection_texname != "")
				{
					 material->reflectionTex->copyImageData(TextureLoader::loadTexture(folder, mat.reflection_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
					 	VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.sheen_texname != "")
				{
					material->sheenTex->copyImageData(TextureLoader::loadTexture(folder, mat.sheen_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}

				if (mat.alpha_texname != "")
				{
					material->alphaTex->copyImageData(TextureLoader::loadTexture(folder, mat.alpha_texname, TextureType::AMBIENT, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
				}
			}

			material->createDescriptorSet(emptyTexture);
			materials.push_back(material);
		}

		std::unordered_map<ModelVertex, uint32_t> uniqueVertices{};

		// read in vertex and index data
		for (tinyobj::shape_t shape : shapes)
		{
			Mesh* newMesh = new Mesh();
			for (tinyobj::index_t index : shape.mesh.indices)
			{
				ModelVertex newVertex{};
				newVertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
					1.0f
				};

				if (index.texcoord_index == -1) // -1 means the texcoord isn't being used 
					newVertex.texcoord = { 0.0f, 0.0f };

				else
					newVertex.texcoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Vulkan Y-axis is flipped, so make sure we adjust textures
				};

				newVertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
					0.0f
				};

				// check if we've already seen this vertex or not
				if (uniqueVertices.count(newVertex) == 0)
				{
					uniqueVertices[newVertex] = static_cast<uint32_t>(newMesh->vertices.size());
					newMesh->vertices.push_back(newVertex);
				}
				newMesh->indices.push_back(uniqueVertices[newVertex]);
			}

			newMesh->vertexBuffer = createMeshVertexBuffer(newMesh->vertices);
			newMesh->indexBuffer = createMeshIndexBuffer(newMesh->indices);

			if (materials.size() > 0)
				newMesh->material = materials[shape.mesh.material_ids[0]];

			else
			{
				newMesh->material = new Material();
				newMesh->material->createDescriptorSet(emptyTexture);
			}

			newMesh->createDescriptorSet();
			meshes.push_back(newMesh);
		}

		return new Model(meshes);
	}

	else
		throw std::runtime_error("Failed to load model!");
}

VulkanBuffer ModelLoader::createMeshVertexBuffer(const std::vector<ModelVertex>& vertices)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	static VkQueue queue = VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue;

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	VulkanBuffer buffer;

	HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.buffer, buffer.bufferMemory);

	HelperFunctions::copyBuffer(shared::commandPool, stagingBuffer, buffer.buffer, static_cast<uint32_t>(bufferSize), queue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	return buffer;
}

VulkanBuffer ModelLoader::createMeshIndexBuffer(const std::vector<uint32_t>& indices)
{
	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();
	static VkQueue queue = VulkanDevice::GetVulkanDevice()->GetQueues().renderQueue;

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	// create a staging buffer to upload vertex data on GPU for high performance
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	VulkanBuffer buffer;

	HelperFunctions::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.buffer, buffer.bufferMemory);

	HelperFunctions::copyBuffer(shared::commandPool, stagingBuffer, buffer.buffer, static_cast<uint32_t>(bufferSize), queue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	return buffer;
}

void ModelLoader::setCommandPool(VkCommandPool& commandPool)
{
	if (shared::commandPool == VK_NULL_HANDLE)
		shared::commandPool = commandPool;
}


void TextureLoader::destroy()
{
	while (!priv::loadedTextures.empty())
	{
		priv::loadedTextures.begin()->second->destroyTexture();
		delete priv::loadedTextures.begin()->second;
		priv::loadedTextures.erase(priv::loadedTextures.begin());
	}

	priv::emptyTexture->destroyTexture();
	delete priv::emptyTexture;
}

void TextureLoader::loadEmptyTexture(VkCommandPool& commandPool)
{
	if (priv::emptyTexture) return;

	VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	//unsigned char pixels[] = {0, 0, 0, 1};
	unsigned char pixels[] = {255, 255, 255, 255};
	int width = 1, height = 1, depth = 1;
	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	VkFilter filter = VK_FILTER_LINEAR;
	VkSamplerAddressMode mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	priv::emptyTexture = new Texture(TextureType::NONE);
	priv::emptyTexture->width = 1;
	priv::emptyTexture->height = 1;
	priv::emptyTexture->mipLevels = 1;
	priv::emptyTexture->layerCount = 1;

	VkDeviceSize imageSize = 4;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	HelperFunctions::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	HelperFunctions::createImage(width, height, 1, 1, VK_SAMPLE_COUNT_1_BIT, 
		imageType, format, tiling, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, priv::emptyTexture->image, priv::emptyTexture->imageMemory);

	HelperFunctions::transitionImageLayout(priv::emptyTexture->image, format, 1, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandPool,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	HelperFunctions::copyBufferToImage(commandPool, stagingBuffer, priv::emptyTexture->image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);

	HelperFunctions::transitionImageLayout(priv::emptyTexture->image, format, 1,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandPool,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	HelperFunctions::createImageView(priv::emptyTexture->image, priv::emptyTexture->imageView, format, aspectFlags, viewType, 1);
	HelperFunctions::createSampler(priv::emptyTexture->sampler, filter, mode, VK_FALSE, 1.0f, 0.0f, 1);

	priv::emptyTexture->textureDescriptor.sampler = priv::emptyTexture->sampler;
	priv::emptyTexture->textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	priv::emptyTexture->textureDescriptor.imageView = priv::emptyTexture->imageView;
}

Texture* TextureLoader::loadTexture(std::string folder, std::string name, TextureType textureType, VkImageType imageType,
	VkFormat format, VkImageTiling tiling, VkImageAspectFlags aspectFlags, VkFilter filter, VkSamplerAddressMode mode)
{
	std::unordered_map<std::string, Texture*>::const_iterator it = priv::loadedTextures.find(name);

	if (it != priv::loadedTextures.end())
		return it->second;

	static VkDevice device = VulkanDevice::GetVulkanDevice()->GetLogicalDevice();

	folder += "/";

	int width, height, channels;
	stbi_uc* pixels = stbi_load((directory + folder + name).c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels)
		return priv::emptyTexture;

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	Texture* tex = new Texture(textureType);
	tex->width = uint32_t(width);
	tex->height = uint32_t(height);
	tex->mipLevels = mipLevels;
	tex->layerCount = 1;
	priv::loadedTextures.insert({ name, tex });

	VkDeviceSize imageSize = width * height * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	HelperFunctions::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	VkImageViewType viewType;
	switch (imageType)
	{
	case VK_IMAGE_TYPE_1D:
		viewType = VK_IMAGE_VIEW_TYPE_1D;
		break;

	case VK_IMAGE_TYPE_2D:
		viewType = VK_IMAGE_VIEW_TYPE_2D;
		break;

	case VK_IMAGE_TYPE_3D:
		viewType = VK_IMAGE_VIEW_TYPE_3D;
		break;
	}

	HelperFunctions::createImage(width, height, 1, mipLevels, VK_SAMPLE_COUNT_1_BIT,
		imageType, format, tiling, 
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex->image, tex->imageMemory);

	HelperFunctions::transitionImageLayout(tex->image, format, mipLevels,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, shared::commandPool,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	uint32_t texWidth = static_cast<uint32_t>(width);
	uint32_t texHeight = static_cast<uint32_t>(height);
	HelperFunctions::copyBufferToImage(shared::commandPool, stagingBuffer, tex->image, texWidth, texHeight, 1);
	
	HelperFunctions::generateImageMipmaps(tex->image, format, texWidth, texHeight, 1, mipLevels, shared::commandPool);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	HelperFunctions::createImageView(tex->image, tex->imageView, format, aspectFlags, viewType, mipLevels);
	HelperFunctions::createSampler(tex->sampler, filter, mode, VK_TRUE, 4.0f, 0.0f, mipLevels);

	tex->textureDescriptor.sampler = tex->sampler;
	tex->textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	tex->textureDescriptor.imageView = tex->imageView;
	return tex;
}

Texture* TextureLoader::getEmptyTexture()
{
	if (!priv::emptyTexture)
		loadEmptyTexture(shared::commandPool);

	return priv::emptyTexture;
}

void TextureLoader::setCommandPool(VkCommandPool& commandPool)
{
	if (shared::commandPool == VK_NULL_HANDLE)
		shared::commandPool = commandPool;
}


