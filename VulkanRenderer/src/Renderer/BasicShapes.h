#pragma once
#include <vulkan/vulkan.h>
#include "HelperStructs.h"

namespace BasicShapes
{
	// commands to simply draw a premade shape
	void drawBox(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void drawSphere(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void drawTorus(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void drawPlane(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void drawCone(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void drawMonkey(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);
	void drawCylinder(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useMaterial = false);

	void loadShapes(VkCommandPool& commandPool);
	void destroyShapes();

	Mesh* getBox();
	Mesh* getPlane();
	Mesh* getSphere();
	Mesh* getTorus();
	Mesh* getCone();
	Mesh* getMonkey();
	Mesh* getCylinder();

	// create instances of a shape
	// currently, only the plane and box are generated manually
	// the rest are loaded as a model
	Mesh createBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f);
	Mesh createPlane(float length = 1.0f, float width = 1.0f);
	Mesh createSphere();
	Mesh createTorus();
	Mesh createCone();
	Mesh createMonkey();
	Mesh createCylinder();
}