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

#ifndef HELPERSTRUCTS_H
#define HELPERSTRUCTS_H

#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <unordered_map>
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include "HelperFunctions.h"

struct VulkanSwapChain
{
	VkSwapchainKHR swapChain{};
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainDimensions;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> surfacePresentModes;
};

struct VulkanBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
	void* mappedMemory = nullptr; // idea taken from Sascha Willem. simplifies repeated VkMapMemory/VkUnmapMemory calls
	bool isMapped = false;
	VkDeviceSize bufferSize = 0;
	VkDeviceSize bufferAlignment = 0;

	void map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void unmap();
	void destroy();
	void flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
};

struct VulkanGraphicsPipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorPool descriptorPool;

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	VkImage depthStencilBuffer;
	VkImageView depthStencilBufferView;
	VkDeviceMemory depthStencilBufferMemory;

	VkViewport viewport;
	VkRect2D scissors;

	// for scenes where vertex data is passed in raw, instead of stored in a mesh
	VulkanBuffer vertexBuffer = {};
	VulkanBuffer indexBuffer = {};

	std::vector<VulkanBuffer> uniformBuffers;

	bool isDepthBufferEmpty = true,
		 isDescriptorPoolEmpty = true;

	size_t shaderFileSize;

	VkResult result;

	void destroyGraphicsPipeline(const VkDevice& device);
	
};

struct VulkanComputePipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	std::optional<VulkanBuffer> storageBuffer, uniformBuffer;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	VkResult result;

	void destroyComputePipeline(const VkDevice& device);
	
};

enum class VulkanReturnValues
{
	VK_SWAPCHAIN_OUT_OF_DATE,
	VK_FUNCTION_SUCCESS,
	VK_FUNCTION_FAILED,
};

// simple vertex structure for position and color
struct Vertex
{
	glm::vec3 position; // for 2D objects, simply ignore the z value
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

// vertex structure for models, contains positions, texcoords, and normals
struct ModelVertex
{
	alignas(16)glm::vec3 position; 
	alignas(8)glm::vec2 texcoord;
	alignas(16)glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	bool operator==(const ModelVertex& other) const;
};

// vertex structure for ImGui fonts
struct FontVertex
{
	alignas(8)glm::vec2 position;
	alignas(8)glm::vec2 texcoord;
	glm::vec4 color;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	bool operator==(const FontVertex& other) const;
};

struct Particle
{
	alignas(16)glm::vec3 position;
	alignas(16)glm::vec3 velocity;
	alignas(16)glm::vec3 color;
};

enum class TextureType
{
	NONE = -1,
	AMBIENT = 1,
	DIFFUSE,
	SPECULAR,
	SPECULAR_HIGHLIGHT,
	NORMAL,
	ALPHA,
	METALLIC,
	DISPLACEMENT,
	EMISSIVE,
	REFLECTION,
	ROUGHNESS,
	SHEEN,
};

struct Texture
{
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory imageMemory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VkDescriptorImageInfo textureDescriptor = {};
	uint32_t width = 0, height = 0;
	uint32_t mipLevels = 0;
	uint32_t layerCount = 0;	
	TextureType type;

	Texture();
	Texture(TextureType textureType);

	bool operator==(const Texture* texture);
	bool operator!=(const Texture* texture);

	void copyImageData(const Texture* texture);
	void destroyTexture();
};

// extended to include PBR values
struct Material 
{
	Material();
	std::string name;

	struct
	{
		alignas(16)glm::vec3 ambient = glm::vec3(0.1f);
		alignas(16)glm::vec3 diffuse = glm::vec3(0.5f);
		alignas(16)glm::vec3 specular = glm::vec3(1.0f);
		alignas(16)glm::vec3 transmittance = glm::vec3(0.0f);
		alignas(16)glm::vec3 emission = glm::vec3(0.0f);

		float shininess = 1.0f;			   // specular exponent
		float ior = 0.0f;				   // index of refraction
		float dissolve = 1.0f;			   // 1 == opaque; 0 == fully transparent 
		int illum = 0;					   // illumination model
		float roughness = 0.0f;            // [0, 1] default 0
		float metallic = 0.0f;             // [0, 1] default 0
		float sheen = 0.0f;                // [0, 1] default 0
		float clearcoat_thickness = 0.0f;  // [0, 1] default 0
		float clearcoat_roughness = 0.0f;  // [0, 1] default 0
		float anisotropy = 0.0f;           // aniso. [0, 1] default 0
		float anisotropy_rotation = 0.0f;  // anisor. [0, 1] default 0
		float pad0 = 0.0f;
		int dummy;						   // Suppress padding warning.
	} ubo;

	// textures						 // MTL values (see tiny_obj_loader.h)
	Texture* ambientTex = nullptr;				 // map_Ka
	Texture* diffuseTex = nullptr;				 // map_Kd
	Texture* specularTex = nullptr;			 // map_Ks
	Texture* specularHighlightTex = nullptr;    // map_Ns
	Texture* normalTex = nullptr;				 // norm
	Texture* alphaTex = nullptr;				 // map_d
	Texture* metallicTex = nullptr;			 // map_Pm
	Texture* displacementTex = nullptr;		 // disp
	Texture* emissiveTex = nullptr;			 // map_Ke
	Texture* reflectionTex = nullptr;			 // refl
	Texture* roughnessTex = nullptr;			 // map_Pr
	Texture* sheenTex = nullptr;				 // map_Ps

	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VulkanBuffer uniformBuffer;

	void destroy();

	void createDescriptorSet(Texture* emptyTexture);
};

enum class ColorType
{
	AMBIENT,
	DIFFUSE,
	SPECULAR,
	TRANSMITTANCE,
	EMISSION
};

enum class MaterialValueType
{
	SHININESS,
	REFRACTION,
	DISSOLVE,
	ROUGHNESS,
	METALLIC,
	SHEEN,
	CC_THICKNESS,
	CC_ROUGHNESS,
	ANISOTROPY,
	ANISOTROPY_ROT
};

enum class MaterialPresets
{
	EMERALD, RUBY, TURQUOISE, JADE, PEARL, OBSIDIAN,
	BRASS, BRONZE, CHROME, COPPER, GOLD, SILVER,
	POLISHED_BRONZE, POLISHED_COPPER, POLISHED_GOLD, POLISHED_SILVER,
	BLACK_PLASTIC, WHITE_PLASTIC,
	RED_PLASTIC, YELLOW_PLASTIC, GREEN_PLASTIC, CYAN_PLASTIC,
	BLACK_RUBBER, WHITE_RUBBER,
	RED_RUBBER, YELLOW_RUBBER, GREEN_RUBBER, CYAN_RUBBER
};

struct Mesh
{
	std::vector<ModelVertex> vertices;
	std::vector<uint32_t> indices;
	VulkanBuffer vertexBuffer, indexBuffer;
	Material* material;

	struct
	{
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 normal = glm::mat4(1.0f);
	} meshUBO;

	VulkanBuffer uniformBuffer;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorPool descriptorPool;

	void createDescriptorSet();
	void destroyMesh();

	void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void setModelMatrix(glm::mat4 m);
	void setMaterialColorWithValue(ColorType colorType, glm::vec3 color);
	void setMaterialWithPreset(MaterialPresets preset);
	void setMaterialValue(MaterialValueType valueType, float value);
};

struct Model
{
	Model(std::vector<Mesh*> modelMeshes) 
		: meshes(modelMeshes){}
	std::vector<Mesh*> meshes;
	void destroyModel();
	void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, bool useMaterial = false);
};

// hash functions
namespace std
{
	template <class T>
	inline void hash_combine(size_t& s, const T& v)
	{
		hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	template<> struct hash<ModelVertex>
	{
		size_t operator()(ModelVertex const& vertex) const
		{
			size_t res = 0;
			hash_combine(res, vertex.position);
			hash_combine(res, vertex.texcoord);
			hash_combine(res, vertex.normal);
			return res;
		}
	};

	template<> struct hash<Texture>
	{
		size_t operator()(Texture const& tex) const
		{
			size_t res = 0;
			hash_combine(res, tex.image);
			hash_combine(res, tex.imageView);
			hash_combine(res, tex.imageMemory);
			hash_combine(res, tex.sampler);
			return res;
		}
	};

	template<> struct hash<Material>
	{
		size_t operator()(Material const& mat) const
		{
			size_t res = 0;
			hash_combine(res, mat.name);
			hash_combine(res, mat.ubo.ambient);
			hash_combine(res, mat.ubo.diffuse);
			hash_combine(res, mat.ubo.specular);
			hash_combine(res, mat.ubo.transmittance);
			hash_combine(res, mat.ubo.emission);
			return res;
		}
	};
}

#endif //!HELPERSTRUCTS_H