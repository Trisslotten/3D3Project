#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include "../entity.h"
#include "../world.h"

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;
	int computeFamily = -1;
	int transferFamily = -1;
	bool isComplete()
	{
		return graphicsFamily >= 0 
			&& presentFamily >= 0 
			&& computeFamily >= 0
			&& transferFamily >= 0;
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
	void init(unsigned char* map, vec2 mapdims);
	//void submit();
	void render();
	void cleanup();

	void submitEntity(Entity e);

	bool windowShouldClose();

	void initCompute(size_t sizeMap, size_t sizeEntites);
	void mapComputeMemory(void* map, void* entities, size_t mapSize, size_t entitySize);
	void createComputePipeline();
private:
	void createWindow();
	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createFramebuffers();
	void createGraphicsPipeline();



	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkShaderModule createShaderModule(const std::string& filepath);

	const int MAX_FRAMES_IN_FLIGHT = 2;
	int width = 800;
	int height = 600;
	GLFWwindow* window;

	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;

	VkDebugReportCallbackEXT callback;

	QueueFamilyIndices familyIndices;



	//compute
	int preComputedSteps = 20;

	VkDeviceMemory computeMemory;
	VkBuffer map_buffer;
	VkBuffer entity_buffer;
	VkBuffer steps_buffer;

	vec2* astarSteps;

	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;

	int alignOffsetEntity = 0;
	int alignOffsetSteps = 0;
};