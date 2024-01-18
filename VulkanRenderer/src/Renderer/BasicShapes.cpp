#include "BasicShapes.h"
#include "Loaders.h"

namespace BasicShapes
{
	namespace shape// for private members and functions
	{
		VkCommandPool commandPool;
		Mesh* box = nullptr;
		Mesh* plane = nullptr;
		Model* sphere = nullptr;
		Model* torus = nullptr;
		Model* cone = nullptr;
		Model* monkey = nullptr;
		Model* cylinder = nullptr;

		void createBox()
		{
			box = new Mesh();

			box->vertices = {
				// front
				{{-0.5f, -0.5f, 0.5f, 1.0f},  {0.0f, 0.0f}, {0.0f,  0.0f, 1.0f, 0.0f}}, // 0
				{{ 0.5f, -0.5f, 0.5f, 1.0f},  {1.0f, 0.0f}, {0.0f,  0.0f, 1.0f, 0.0f}}, // 1
				{{ 0.5f,  0.5f, 0.5f, 1.0f},  {1.0f, 1.0f}, {0.0f,  0.0f, 1.0f, 0.0f}}, // 2
				{{-0.5f,  0.5f, 0.5f, 1.0f},  {0.0f, 1.0f}, {0.0f,  0.0f, 1.0f, 0.0f}}, // 3

				// back
				{{ 0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f}, {0.0f,  0.0f,  -1.0f, 0.0f}}, // 4
				{{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f}, {0.0f,  0.0f,  -1.0f, 0.0f}}, // 5
				{{-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f,  0.0f,  -1.0f, 0.0f}}, // 6
				{{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f,  0.0f,  -1.0f, 0.0f}}, // 7

				// left
				{{-0.5f, -0.5f, -0.5f, 1.0f},  {0.0f, 0.0f}, {-1.0f, 0.0f,  0.0f, 0.0f}}, // 8
				{{-0.5f, -0.5f,  0.5f, 1.0f},  {1.0f, 0.0f}, {-1.0f, 0.0f,  0.0f, 0.0f}}, // 9
				{{-0.5f,  0.5f,  0.5f, 1.0f},  {1.0f, 1.0f}, {-1.0f, 0.0f,  0.0f, 0.0f}}, // 10
				{{-0.5f,  0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, {-1.0f, 0.0f,  0.0f, 0.0f}}, // 11

				// right
				{{ 0.5f, -0.5f,  0.5f, 1.0f},  {0.0f, 0.0f}, { 1.0f, 0.0f,  0.0f, 0.0f}}, // 12
				{{ 0.5f, -0.5f, -0.5f, 1.0f},  {1.0f, 0.0f}, { 1.0f, 0.0f,  0.0f, 0.0f}}, // 13
				{{ 0.5f,  0.5f, -0.5f, 1.0f},  {1.0f, 1.0f}, { 1.0f, 0.0f,  0.0f, 0.0f}}, // 14
				{{ 0.5f,  0.5f,  0.5f, 1.0f},  {0.0f, 1.0f}, { 1.0f, 0.0f,  0.0f, 0.0f}}, // 15

				// top
				{{-0.5f,  0.5f,  0.5f, 1.0f},  {0.0f, 0.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}}, // 16
				{{ 0.5f,  0.5f,  0.5f, 1.0f},  {1.0f, 0.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}}, // 17
				{{ 0.5f,  0.5f, -0.5f, 1.0f},  {1.0f, 1.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}}, // 18
				{{-0.5f,  0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}}, // 19

				// bottom
				{{ 0.5f, -0.5f,  0.5f, 1.0f},  {0.0f, 0.0f}, { 0.0f,-1.0f,  0.0f, 0.0f}}, // 20
				{{-0.5f, -0.5f,  0.5f, 1.0f},  {1.0f, 0.0f}, { 0.0f,-1.0f,  0.0f, 0.0f}}, // 21
				{{-0.5f, -0.5f, -0.5f, 1.0f},  {1.0f, 1.0f}, { 0.0f,-1.0f,  0.0f, 0.0f}}, // 22
				{{ 0.5f, -0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, { 0.0f,-1.0f,  0.0f, 0.0f}}, // 23
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

			box->material->createDescriptorSet(emptyTexture);

			box->createDescriptorSet();
		}

		void createSphere()
		{
			sphere = ModelLoader::loadModel("BasicShapes", "basic_sphere.obj");
		}

		void createTorus()
		{
			torus = ModelLoader::loadModel("BasicShapes", "basic_torus.obj");
		}

		void createPlane()
		{
			plane = new Mesh();
			plane->vertices = {
				{{-0.5f,  0.0f,  0.5f, 1.0f},  {0.0f, 0.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}},
				{{ 0.5f,  0.0f,  0.5f, 1.0f},  {1.0f, 0.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}},
				{{ 0.5f,  0.0f, -0.5f, 1.0f},  {1.0f, 1.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}},
				{{-0.5f,  0.0f, -0.5f, 1.0f},  {0.0f, 1.0f}, { 0.0f, 1.0f,  0.0f, 0.0f}}
			};

			plane->indices = { 0, 1, 2, 0, 2, 3 };
			plane->vertexBuffer = ModelLoader::createMeshVertexBuffer(plane->vertices);
			plane->indexBuffer = ModelLoader::createMeshIndexBuffer(plane->indices);

			Texture* emptyTexture = TextureLoader::getEmptyTexture();

			plane->material = new Material();

			plane->material->createDescriptorSet(emptyTexture);
			plane->createDescriptorSet();

		}

		void createCone()
		{
			cone = ModelLoader::loadModel("BasicShapes", "basic_cone.obj");
		}

		void createMonkey()
		{
			monkey = ModelLoader::loadModel("BasicShapes", "basic_monkey.obj");
		}

		void createCylinder()
		{
			cylinder = ModelLoader::loadModel("BasicShapes", "basic_cylinder.obj");
		}
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
	void destroyShapes()
	{
		if (shape::box)
		{
			shape::box->destroyMesh();
			delete shape::box;
		}

		if (shape::sphere)
		{
			delete shape::sphere;
		}

		if (shape::torus)
		{
			delete shape::torus;
		}

		if (shape::plane)
		{
			shape::plane->destroyMesh();
			delete shape::plane;
		}

		if (shape::cone)
		{
			delete shape::cone;
		}
		
		if (shape::monkey)
		{
			delete shape::monkey;
		}
		
		if (shape::cylinder)
		{
			delete shape::cylinder;
		}
	}
	
	Mesh* getBox() 
	{ 
		if (!shape::box)
			shape::createBox();

		return shape::box; 
	}
	Mesh* getSphere() 
	{ 
		if (!shape::sphere)
			shape::createSphere();

		return shape::sphere->meshes[0]; 
	}
	Mesh* getTorus() 
	{ 
		if (!shape::torus)
			shape::createTorus();

		return shape::torus->meshes[0]; 
	}
	Mesh* getPlane() 
	{ 
		if (!shape::plane)
			shape::createPlane();

		return shape::plane; 
	}
	Mesh* getCone() 
	{
		if (!shape::cone)
			shape::createCone();

		return shape::cone->meshes[0]; 
	}
	Mesh* getMonkey() 
	{ 
		if (!shape::monkey)
			shape::createMonkey();

		return shape::monkey->meshes[0]; 
	}
	Mesh* getCylinder() 
	{
		if (!shape::cylinder)
			shape::createCylinder();

		return shape::cylinder->meshes[0]; 
	}

	Mesh createBox(float width, float height, float depth)
	{
		// halve the dimensions to produce an accurately sized box
		// for example, using 1, 1, 1, the width/height/depth would go between -1 and 1,
		// effectively making the size of each dimension 2
		float w = width / 2.0f;
		float h = height / 2.0f;
		float d = depth / 2.0f;

		Mesh box = {};

		box.vertices = {
			// front
			{{-w, -h,  d, 1.0f},  {0.0f, 0.0f}, { 0.0f,  0.0f,   1.0f, 0.0f}}, // 0
			{{ w, -h,  d, 1.0f},  {1.0f, 0.0f}, { 0.0f,  0.0f,   1.0f, 0.0f}}, // 1
			{{ w,  h,  d, 1.0f},  {1.0f, 1.0f}, { 0.0f,  0.0f,   1.0f, 0.0f}}, // 2
			{{-w,  h,  d, 1.0f},  {0.0f, 1.0f}, { 0.0f,  0.0f,   1.0f, 0.0f}}, // 3

			// back
			{{ w, -h, -d, 1.0f},  {0.0f, 0.0f}, { 0.0f,  0.0f,  -1.0f, 0.0f}}, // 4
			{{-w, -h, -d, 1.0f},  {1.0f, 0.0f}, { 0.0f,  0.0f,  -1.0f, 0.0f}}, // 5
			{{-w,  h, -d, 1.0f},  {1.0f, 1.0f}, { 0.0f,  0.0f,  -1.0f, 0.0f}}, // 6
			{{ w,  h, -d, 1.0f},  {0.0f, 1.0f}, { 0.0f,  0.0f,  -1.0f, 0.0f}}, // 7

			// left
			{{-w, -h, -d, 1.0f},  {0.0f, 0.0f}, {-1.0f,  0.0f,   0.0f, 0.0f}}, // 8
			{{-w, -h,  d, 1.0f},  {1.0f, 0.0f}, {-1.0f,  0.0f,   0.0f, 0.0f}}, // 9
			{{-w,  h,  d, 1.0f},  {1.0f, 1.0f}, {-1.0f,  0.0f,   0.0f, 0.0f}}, // 10
			{{-w,  h, -d, 1.0f},  {0.0f, 1.0f}, {-1.0f,  0.0f,   0.0f, 0.0f}}, // 11

			// right
			{{ w, -h,  d, 1.0f},  {0.0f, 0.0f}, { 1.0f,  0.0f,   0.0f, 0.0f}}, // 12
			{{ w, -h, -d, 1.0f},  {1.0f, 0.0f}, { 1.0f,  0.0f,   0.0f, 0.0f}}, // 13
			{{ w,  h, -d, 1.0f},  {1.0f, 1.0f}, { 1.0f,  0.0f,   0.0f, 0.0f}}, // 14
			{{ w,  h,  d, 1.0f},  {0.0f, 1.0f}, { 1.0f,  0.0f,   0.0f, 0.0f}}, // 15

			// top
			{{-w,  h,  d, 1.0f},  {0.0f, 0.0f}, { 0.0f,  1.0f,   0.0f, 0.0f}}, // 16
			{{ w,  h,  d, 1.0f},  {1.0f, 0.0f}, { 0.0f,  1.0f,   0.0f, 0.0f}}, // 17
			{{ w,  h, -d, 1.0f},  {1.0f, 1.0f}, { 0.0f,  1.0f,   0.0f, 0.0f}}, // 18
			{{-w,  h, -d, 1.0f},  {0.0f, 1.0f}, { 0.0f,  1.0f,   0.0f, 0.0f}}, // 19

			// bottom
			{{ w, -h,  d, 1.0f},  {0.0f, 0.0f}, { 0.0f, -1.0f,   0.0f, 0.0f}}, // 20
			{{-w, -h,  d, 1.0f},  {1.0f, 0.0f}, { 0.0f, -1.0f,   0.0f, 0.0f}}, // 21
			{{-w, -h, -d, 1.0f},  {1.0f, 1.0f}, { 0.0f, -1.0f,   0.0f, 0.0f}}, // 22
			{{ w, -h, -d, 1.0f},  {0.0f, 1.0f}, { 0.0f, -1.0f,   0.0f, 0.0f}}, // 23
		};
		box.indices = {
			0, 1, 2, 0, 2, 3,       // front
			4, 5, 6, 4, 6, 7,       // back
			8, 9, 10, 8, 10, 11,    // left
			12, 13, 14, 12, 14, 15, // right
			16, 17, 18, 16, 18, 19, // top
			20, 21, 22, 20, 22, 23  // bottom
		};

		box.vertexBuffer = ModelLoader::createMeshVertexBuffer(box.vertices);
		box.indexBuffer = ModelLoader::createMeshIndexBuffer(box.indices);

		box.material = new Material();
		box.material->ubo.ambient = glm::vec3(0.1f);
		box.material->ubo.diffuse = glm::vec3(0.8f, 0.2f, 0.6f);

		box.material->createDescriptorSet(TextureLoader::getEmptyTexture());

		box.createDescriptorSet();

		return box;
	}

	Mesh createPlane(float length, float width)
	{
		Mesh plane = {};
		float l = length / 2.0f;
		float w = width / 2.0f;

		plane.vertices = {
			{{-l,  0.0f,  w, 1.0f},  {0.0f, 0.0f}, { 0.0f, 1.0f, 0.0f, 0.0f}},
			{{ l,  0.0f,  w, 1.0f},  {1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f, 0.0f}},
			{{ l,  0.0f, -w, 1.0f},  {1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f, 0.0f}},
			{{-l,  0.0f, -w, 1.0f},  {0.0f, 1.0f}, { 0.0f, 1.0f, 0.0f, 0.0f}}
		};

		plane.indices = { 0, 1, 2, 0, 2, 3 };
		plane.vertexBuffer = ModelLoader::createMeshVertexBuffer(plane.vertices);
		plane.indexBuffer = ModelLoader::createMeshIndexBuffer(plane.indices);

		plane.material = new Material();

		plane.material->createDescriptorSet(TextureLoader::getEmptyTexture());

		plane.createDescriptorSet();

		return plane;
	}

	Mesh createSphere()
	{
		Mesh sphere = {};
		Mesh* sphereModel = getSphere();

		sphere.vertices = sphereModel->vertices;
		sphere.indices = sphereModel->indices;
		sphere.vertexBuffer = ModelLoader::createMeshVertexBuffer(sphere.vertices);
		sphere.indexBuffer = ModelLoader::createMeshIndexBuffer(sphere.indices);

		sphere.material = new Material();
		sphere.material->createDescriptorSet(TextureLoader::getEmptyTexture());
		sphere.createDescriptorSet();

		return sphere;
	}

	Mesh createTorus()
	{
		Mesh torus = {};
		Mesh* torusModel = getTorus();

		torus.vertices = torusModel->vertices;
		torus.indices = torusModel->indices;
		torus.vertexBuffer = ModelLoader::createMeshVertexBuffer(torus.vertices);
		torus.indexBuffer = ModelLoader::createMeshIndexBuffer(torus.indices);
		torus.material = new Material();
		torus.material->createDescriptorSet(TextureLoader::getEmptyTexture());
		torus.createDescriptorSet();

		return torus;
	}

	Mesh createCone()
	{
		Mesh cone = {};
		Mesh* coneModel = getCone();

		cone.vertices = coneModel->vertices;
		cone.indices = coneModel->indices;
		cone.vertexBuffer = ModelLoader::createMeshVertexBuffer(cone.vertices);
		cone.indexBuffer = ModelLoader::createMeshIndexBuffer(cone.indices);
		cone.material = new Material();
		cone.material->createDescriptorSet(TextureLoader::getEmptyTexture());
		cone.createDescriptorSet();

		return cone;
	}

	Mesh createMonkey()
	{
		Mesh monkey = {};
		Mesh* monkeyModel = getMonkey();

		monkey.vertices = monkeyModel->vertices;
		monkey.indices = monkeyModel->indices;
		monkey.vertexBuffer = ModelLoader::createMeshVertexBuffer(monkey.vertices);
		monkey.indexBuffer = ModelLoader::createMeshIndexBuffer(monkey.indices);
		monkey.material = new Material();
		monkey.material->createDescriptorSet(TextureLoader::getEmptyTexture());
		monkey.createDescriptorSet();

		return monkey;
	}

	Mesh createCylinder()
	{
		Mesh cylinder = {};
		Mesh* cylinderModel = getCylinder();

		cylinder.vertices = cylinderModel->vertices;
		cylinder.indices = cylinderModel->indices;
		cylinder.vertexBuffer = ModelLoader::createMeshVertexBuffer(cylinder.vertices);
		cylinder.indexBuffer = ModelLoader::createMeshIndexBuffer(cylinder.indices);
		cylinder.material = new Material();
		cylinder.material->createDescriptorSet(TextureLoader::getEmptyTexture());
		cylinder.createDescriptorSet();

		return cylinder;
	}
}

