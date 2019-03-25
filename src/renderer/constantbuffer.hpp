#pragma once

#include <vulkan/vulkan.h>

class ConstantBufferVK
{
public:
	ConstantBufferVK(size_t size, VkDevice device, VkPhysicalDevice pdevice);
	~ConstantBufferVK();
	void setData(const void* data, size_t size);
	
	VkDescriptorSetLayoutBinding uboLayoutBinding;
	VkBuffer _handle;
	size_t size;
private:
	void init();
	//uint32_t handle;
	uint32_t index;
	void* buff = nullptr;
	void* lastMat;
	VkDevice _device;
	VkPhysicalDevice _physicalDevice;
	VkDeviceMemory cBufferMemory;
};

