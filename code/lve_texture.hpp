#pragma once

#include "lve_device.hpp"
#include "lve_buffer.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace lve {
	class LveTexture {
	public:
		LveTexture(LveDevice& device, const char * filePath);
		~LveTexture();

		LveTexture(const LveTexture&) = delete;
		LveTexture& operator=(const LveTexture&) = delete;


		VkImageView textureImageView;
		VkSampler textureSampler;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;

private:
	void createTextureImage(const char* filePath);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createImage(uint32_t texWidth, uint32_t texHeight, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& imageMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createTextureImageView();
	VkImageView createImageView(VkImage image, VkFormat format);
	void createTextureSampler();

	LveDevice& lveDevice;

	std::unique_ptr<LveBuffer> pixelBuffer;
	uint32_t pixelCount;



	};
}