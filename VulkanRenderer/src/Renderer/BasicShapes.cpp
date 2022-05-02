#include "BasicShapes.h"
#include "Loaders.h"

namespace BasicShapes
{
	namespace shape// for private members and functions
	{
		void createBox();
		void createSphere();
		void createTorus();
		void createPlane();

		VkCommandPool commandPool;
		Mesh* box = nullptr;
		Mesh* sphere = nullptr;
		Mesh* plane = nullptr;
		Mesh* torus = nullptr;
	}

	void drawBox(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors)
	{
		if (shape::box == nullptr)
			shape::createBox();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shape::box->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, shape::box->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		if (useDescriptors)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				1, 1, &shape::box->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(shape::box->indices.size()), 1, 0, 0, 0);
	}

	void drawSphere(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors)
	{
		if (shape::sphere == nullptr)
			shape::createSphere();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shape::sphere->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, shape::sphere->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		if (useDescriptors)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				1, 1, &shape::sphere->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(shape::sphere->indices.size()), 1, 0, 0, 0);
	}

	void drawTorus(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors)
	{
		if (shape::torus == nullptr)
			shape::createTorus();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shape::torus->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, shape::torus->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		if (useDescriptors)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				1, 1, &shape::torus->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(shape::torus->indices.size()), 1, 0, 0, 0);
	}

	void drawPlane(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors)
	{
		if (shape::plane == nullptr)
			shape::createPlane();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shape::plane->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, shape::plane->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		if (useDescriptors)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				1, 1, &shape::plane->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(shape::plane->indices.size()), 1, 0, 0, 0);
	}

	void loadShapes(VkCommandPool& commandPool)
	{
		shape::commandPool = commandPool;
		if (!shape::box)    shape::createBox();
		if (!shape::sphere) shape::createSphere();
		if (!shape::torus)  shape::createTorus();
		if (!shape::plane)  shape::createPlane();
	}

	void shape::createBox()
	{
		box = new Mesh();

		box->vertices = {
			// front
			{{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f}, {0.0f,  0.0f, 1.0f}}, // 0
			{{ 0.5f, -0.5f, 0.5f},  {1.0f, 0.0f}, {0.0f,  0.0f, 1.0f}}, // 1
			{{ 0.5f,  0.5f, 0.5f},  {1.0f, 1.0f}, {0.0f,  0.0f, 1.0f}}, // 2
			{{-0.5f,  0.5f, 0.5f},  {0.0f, 1.0f}, {0.0f,  0.0f, 1.0f}}, // 3

			// back
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f,  0.0f,  -1.0f}}, // 4
			{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f,  0.0f,  -1.0f}}, // 5
			{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f,  0.0f,  -1.0f}}, // 6
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f,  0.0f,  -1.0f}}, // 7

			// left
			{{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f}, {-1.0f, 0.0f,  0.0f}}, // 8
			{{-0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}, {-1.0f, 0.0f,  0.0f}}, // 9
			{{-0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}, {-1.0f, 0.0f,  0.0f}}, // 10
			{{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}, {-1.0f, 0.0f,  0.0f}}, // 11

			// right
			{{ 0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}, { 1.0f, 0.0f,  0.0f}}, // 12
			{{ 0.5f, -0.5f, -0.5f},	{1.0f, 0.0f}, { 1.0f, 0.0f,  0.0f}}, // 13
			{{ 0.5f,  0.5f, -0.5f},	{1.0f, 1.0f}, { 1.0f, 0.0f,  0.0f}}, // 14
			{{ 0.5f,  0.5f,  0.5f},  {0.0f, 1.0f}, { 1.0f, 0.0f,  0.0f}}, // 15

			// top
			{{-0.5f,  0.5f,  0.5f},  {0.0f, 0.0f}, { 0.0f, 1.0f,  0.0f}}, // 16
			{{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}, { 0.0f, 1.0f,  0.0f}}, // 17
			{{ 0.5f,  0.5f, -0.5f},	 {1.0f, 1.0f}, { 0.0f, 1.0f,  0.0f}}, // 18
			{{-0.5f,  0.5f, -0.5f},	 {0.0f, 1.0f}, { 0.0f, 1.0f,  0.0f}}, // 19

			// bottom
			{{ 0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}, { 0.0f,-1.0f,  0.0f}}, // 20
			{{-0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}, { 0.0f,-1.0f,  0.0f}}, // 21
			{{-0.5f, -0.5f, -0.5f},	 {1.0f, 1.0f}, { 0.0f,-1.0f,  0.0f}}, // 22
			{{ 0.5f, -0.5f, -0.5f},	 {0.0f, 1.0f}, { 0.0f,-1.0f,  0.0f}}, // 23
		};
		box->indices = {
			0, 1, 2, 0, 2, 3,       // front
			4, 5, 6, 4, 6, 7,       // back
			8, 9, 10, 8, 10, 11,    // left
			12, 13, 14, 12, 14, 15, // right
			16, 17, 18, 16, 18, 19, // top
			20, 21, 22, 20, 22, 23  // bottom
		};

		box->vertexBuffer = ModelLoader::createMeshVertexBuffer(box->vertices, commandPool);
		box->indexBuffer = ModelLoader::createMeshIndexBuffer(box->indices, commandPool);

		Texture* emptyTexture = TextureLoader::loadEmptyTexture(commandPool);

		box->material = new Material();
		box->material->ubo.ambient = glm::vec3(0.1f);
		box->material->ubo.diffuse = glm::vec3(0.8f, 0.2f, 0.6f);

		box->material->ambientTex->copyImageData(emptyTexture);
		box->material->diffuseTex->copyImageData(emptyTexture);
		box->material->specularTex->copyImageData(emptyTexture);
		box->material->specularHighlightTex->copyImageData(emptyTexture);
		box->material->normalTex->copyImageData(emptyTexture);
		box->material->alphaTex->copyImageData(emptyTexture);
		box->material->metallicTex->copyImageData(emptyTexture);
		box->material->displacementTex->copyImageData(emptyTexture);
		box->material->emissiveTex->copyImageData(emptyTexture);
		box->material->reflectionTex->copyImageData(emptyTexture);
		box->material->roughnessTex->copyImageData(emptyTexture);
		box->material->sheenTex->copyImageData(emptyTexture);

		box->createDescriptorSet(TextureLoader::loadEmptyTexture(commandPool));
	}

	void shape::createSphere()
	{
		sphere = new Mesh();
	}

	void shape::createTorus()
	{
		torus = new Mesh();
	}

	void shape::createPlane()
	{
		plane = new Mesh();
		plane->vertices = {
			{{-0.5f,  0.0f,  0.5f},  {0.0f, 0.0f}, { 0.0f, 1.0f,  0.0f}},
			{{ 0.5f,  0.0f,  0.5f},  {1.0f, 0.0f}, { 0.0f, 1.0f,  0.0f}},
			{{ 0.5f,  0.0f, -0.5f},	 {1.0f, 1.0f}, { 0.0f, 1.0f,  0.0f}},
			{{-0.5f,  0.0f, -0.5f},	 {0.0f, 1.0f}, { 0.0f, 1.0f,  0.0f}},
		};

		plane->indices = { 0, 1, 2, 0, 2, 3 };
		plane->vertexBuffer = ModelLoader::createMeshVertexBuffer(plane->vertices, commandPool);
		plane->indexBuffer = ModelLoader::createMeshIndexBuffer(plane->indices, commandPool);

		Texture* emptyTexture = TextureLoader::loadEmptyTexture(commandPool);

		plane->material = new Material();
		plane->material->ubo.ambient = glm::vec3(0.1f);
		plane->material->ubo.diffuse = glm::vec3(0.8f, 0.2f, 0.6f);

		plane->material->ambientTex->copyImageData(emptyTexture);
		plane->material->diffuseTex->copyImageData(emptyTexture);
		plane->material->specularTex->copyImageData(emptyTexture);
		plane->material->specularHighlightTex->copyImageData(emptyTexture);
		plane->material->normalTex->copyImageData(emptyTexture);
		plane->material->alphaTex->copyImageData(emptyTexture);
		plane->material->metallicTex->copyImageData(emptyTexture);
		plane->material->displacementTex->copyImageData(emptyTexture);
		plane->material->emissiveTex->copyImageData(emptyTexture);
		plane->material->reflectionTex->copyImageData(emptyTexture);
		plane->material->roughnessTex->copyImageData(emptyTexture);
		plane->material->sheenTex->copyImageData(emptyTexture);

		plane->createDescriptorSet(TextureLoader::loadEmptyTexture(commandPool));
	}


	Mesh* getBox() { return shape::box; }
	Mesh* getSphere() { return shape::sphere; }
	Mesh* getTorus() { return shape::torus; }
	Mesh* getPlane() { return shape::plane; }

	void destroyShapes()
	{
		if (shape::box)
		{
			shape::box->destroyMesh();
			delete shape::box;
		}

		if (shape::sphere)
		{
			shape::sphere->destroyMesh();
			delete shape::sphere;
		}

		if (shape::torus)
		{
			shape::torus->destroyMesh();
			delete shape::torus;
		}

		if (shape::plane)
		{
			shape::plane->destroyMesh();
			delete shape::plane;
		}
	}
}

