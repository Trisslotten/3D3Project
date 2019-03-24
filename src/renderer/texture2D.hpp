#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "../entity.h"

class Renderer;


// NOTE: only supports R8!
class Texture2D
{
	friend class Renderer;
	Renderer* renderer;

	unsigned int width, height;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;


	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkImageView createImageView(VkImage image, VkFormat format);

	void createTextureImageView();


public:

	Texture2D(Renderer* renderer) : renderer(renderer)
	{
	}
	void loadFromFile(const std::string& filepath);
};