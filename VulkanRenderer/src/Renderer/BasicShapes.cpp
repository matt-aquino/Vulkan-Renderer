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
		void createCone();
		void createMonkey();
		void createCylinder();

		VkCommandPool commandPool;
		Mesh* box = nullptr;
		Mesh* plane = nullptr;
		Model* sphere = nullptr;
		Model* torus = nullptr;
		Model* cone = nullptr;
		Model* monkey = nullptr;
		Model* cylinder = nullptr;
	}

	void drawBox(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::box == nullptr)
			shape::createBox();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shape::box->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, shape::box->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// bind model uniform buffer
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &shape::box->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &shape::box->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(shape::box->indices.size()), 1, 0, 0, 0);
	}

	void drawSphere(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::sphere == nullptr)
			shape::createSphere();

		Mesh* sphere = getSphere();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sphere->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, sphere->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &sphere->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &sphere->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sphere->indices.size()), 1, 0, 0, 0);
	}

	void drawTorus(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::torus == nullptr)
			shape::createTorus();

		Mesh* torus = getTorus();
		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &torus->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, torus->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// bind model uniform buffer
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &torus->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &torus->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(torus->indices.size()), 1, 0, 0, 0);
	}

	void drawPlane(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::plane == nullptr)
			shape::createPlane();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shape::plane->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, shape::plane->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &shape::plane->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &shape::plane->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(shape::plane->indices.size()), 1, 0, 0, 0);
	}
	
	void drawCone(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::cone == nullptr)
			shape::createCone();

		Mesh* cone = getCone();
		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cone->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, cone->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &cone->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &cone->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(cone->indices.size()), 1, 0, 0, 0);
	}
	
	void drawMonkey(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::monkey == nullptr)
			shape::createMonkey();

		Mesh* monkey = getMonkey();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &monkey->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, monkey->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &monkey->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &monkey->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(monkey->indices.size()), 1, 0, 0, 0);
	}
	
	void drawCylinder(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial)
	{
		if (shape::cylinder == nullptr)
			shape::createCylinder();

		Mesh* cylinder = getCylinder();

		static VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cylinder->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, cylinder->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			1, 1, &cylinder->descriptorSet, 0, nullptr);

		if (useMaterial)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				2, 1, &cylinder->material->descriptorSet, 0, nullptr);
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(cylinder->indices.size()), 1, 0, 0, 0);
	}
	

	void loadShapes(VkCommandPool& commandPool)
	{
		shape::commandPool = commandPool;
		if (!shape::box)      shape::createBox();
		if (!shape::sphere)   shape::createSphere();
		if (!shape::torus)    shape::createTorus();
		if (!shape::plane)    shape::createPlane();
		if (!shape::cone)	  shape::createCone();
		if (!shape::monkey)	  shape::createMonkey();
		if (!shape::cylinder) shape::createCylinder();
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

		box->vertexBuffer = ModelLoader::createMeshVertexBuffer(box->vertices);
		box->indexBuffer = ModelLoader::createMeshIndexBuffer(box->indices);

		Texture* emptyTexture = TextureLoader::getEmptyTexture();

		box->material = new Material();
		box->material->ubo.ambient = glm::vec3(0.1f);
		box->material->ubo.diffuse = glm::vec3(0.8f, 0.2f, 0.6f);

		box->material->createDescriptorSet(emptyTexture);

		box->createDescriptorSet();
	}

	void shape::createSphere()
	{
		sphere = ModelLoader::loadModel("BasicShapes", "basic_sphere.obj");
	}

	void shape::createTorus()
	{
		torus = ModelLoader::loadModel("BasicShapes", "basic_torus.obj");
	}

	void shape::createPlane()
	{
		plane = new Mesh();
		plane->vertices = {
			{{-0.5f,  0.0f,  0.5f},  {0.0f, 0.0f}, { 0.0f, 1.0f,  0.0f}},
			{{ 0.5f,  0.0f,  0.5f},  {1.0f, 0.0f}, { 0.0f, 1.0f,  0.0f}},
			{{ 0.5f,  0.0f, -0.5f},	 {1.0f, 1.0f}, { 0.0f, 1.0f,  0.0f}},
			{{-0.5f,  0.0f, -0.5f},	 {0.0f, 1.0f}, { 0.0f, 1.0f,  0.0f}}
		};

		plane->indices = { 0, 1, 2, 0, 2, 3 };
		plane->vertexBuffer = ModelLoader::createMeshVertexBuffer(plane->vertices);
		plane->indexBuffer = ModelLoader::createMeshIndexBuffer(plane->indices);

		Texture* emptyTexture = TextureLoader::getEmptyTexture();

		plane->material = new Material();
		plane->material->ubo.ambient = glm::vec3(0.1f);
		plane->material->ubo.diffuse = glm::vec3(1.0f);

		plane->material->createDescriptorSet(emptyTexture);

		plane->createDescriptorSet();
	}

	void shape::createCone()
	{
		cone = ModelLoader::loadModel("BasicShapes", "basic_cone.obj");
	}
	
	void shape::createMonkey()
	{
		monkey = ModelLoader::loadModel("BasicShapes", "basic_monkey.obj");
	}
	
	void shape::createCylinder()
	{
		cylinder = ModelLoader::loadModel("BasicShapes", "basic_cylinder.obj");
	}

	Mesh* getBox() { return shape::box; }
	Mesh* getSphere() { return shape::sphere->meshes[0]; }
	Mesh* getTorus() { return shape::torus->meshes[0]; }
	Mesh* getPlane() { return shape::plane; }
	Mesh* getCone() { return shape::cone->meshes[0]; }
	Mesh* getMonkey() { return shape::monkey->meshes[0]; }
	Mesh* getCylinder() { return shape::cylinder->meshes[0]; }

	void destroyShapes()
	{
		if (shape::box)
		{
			shape::box->destroyMesh();
			delete shape::box;
		}

		if (shape::sphere)
		{
			shape::sphere->destroyModel();
			delete shape::sphere;
		}

		if (shape::torus)
		{
			shape::torus->destroyModel();
			delete shape::torus;
		}

		if (shape::plane)
		{
			shape::plane->destroyMesh();
			delete shape::plane;
		}

		if (shape::cone)
		{
			shape::cone->destroyModel();
			delete shape::cone;
		}
		
		if (shape::monkey)
		{
			shape::monkey->destroyModel();
			delete shape::monkey;
		}
		
		if (shape::cylinder)
		{
			shape::cylinder->destroyModel();
			delete shape::cylinder;
		}
	}
}

