#pragma once
#include <GLFW/glfw3.h>
#include <vector>

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;
	int computeFamily = -1;
	bool isComplete()
	{
		return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0;
	}
};
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Renderer
{
public:
	void init(GLFWwindow* window);

	void cleanup();
private:
	void createInstance();
	void createSurface(GLFWwindow* window);
	void pickPhysicalDevice();
	void createLogicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkSurfaceKHR surface;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;

	VkDebugReportCallbackEXT callback;

	QueueFamilyIndices familyIndices;
};