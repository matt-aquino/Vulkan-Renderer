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

#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "HelperStructs.h"

const std::string directory = "assets/models/";

namespace ModelLoader
{
	Model* loadModel(std::string folder, std::string file, VkCommandPool& commandPool);
	VulkanBuffer createMeshVertexBuffer(const std::vector<ModelVertex>& vertices, VkCommandPool& commandPool);
	VulkanBuffer createMeshIndexBuffer(const std::vector<uint32_t>& indices, VkCommandPool& commandPool);

	void destroy();
}

/*
class ModelLoader
{
public:
	static ModelLoader* GetModelLoader();

	

	void destroy();

private:
	ModelLoader();

	static ModelLoader* instance;

};
*/

namespace TextureLoader
{
	Texture* loadTexture(VkCommandPool& commandPool, std::string folder, std::string name, TextureType textureType, VkImageType viewType, VkFormat format, VkImageTiling tiling,
		VkImageAspectFlags aspectFlags, VkFilter filter, VkSamplerAddressMode mode);
	Texture* loadEmptyTexture(VkCommandPool& commandPool);

	void destroy();

	static Texture* emptyTexture = nullptr;
	static std::unordered_map<std::string, Texture*> loadedTextures;
}

/*
class TextureLoader
{
public:
	static TextureLoader* GetTextureLoader();

	

private:
	TextureLoader();
	static TextureLoader* instance;

	void destroyTextures();

	Texture* emptyTexture = nullptr;
	std::unordered_map<std::string, Texture*> loadedTextures;
};
*/
