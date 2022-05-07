#pragma once
#include <vulkan/vulkan.h>
#include "HelperStructs.h"

namespace BasicShapes
{
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
}