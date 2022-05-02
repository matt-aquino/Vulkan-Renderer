#pragma once
#include <vulkan/vulkan.h>
#include "HelperStructs.h"

namespace BasicShapes
{
	void drawBox(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors = false);
	void drawSphere(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors = false);
	void drawTorus(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors = false);
	void drawPlane(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, bool useDescriptors = false);
	void loadShapes(VkCommandPool& commandPool);
	void destroyShapes();

	Mesh* getBox();
	Mesh* getSphere();
	Mesh* getTorus();
	Mesh* getPlane();
}