#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include "../entity.h"
#include "../world.h"
#include "../util/Threadpool.h"
#include "../util/timer.hpp"
#include "texture2D.hpp"

extern int GLOBAL_NUM_THREADS;

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
	Renderer() : 
		threadPool(GLOBAL_NUM_THREADS), 
		texture(this)
	{}

	void init(const std::string& map);
	void render();
	void cleanup();

	void submitEntity(Entity e);

	bool windowShouldClose();

	void initCompute(size_t sizeMap, size_t sizeEntites);
	void mapComputeMemory(void* map, void* entities, size_t mapSize, size_t entitySize);
	void executeCompute();
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
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	void createGraphicsPipeline();
	void createCommandPools();
	void createCommandBuffers();
	void createSyncObjects();
	void createSampler();

	void createComputePipeline();
	void createComputeCommandPools();
	void createComputeDescriptorSets();


	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkShaderModule createShaderModule(const std::string& filepath);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	int commandIndex(int threadID)
	{
		return threadID + currentFrame * GLOBAL_NUM_THREADS;
	}

	friend class Texture2D;

	const int MAX_FRAMES_IN_FLIGHT = 2;
	size_t currentFrame = 0;
	int width = 800;
	int height = 600;
	GLFWwindow* window;
	threadpool::Threadpool threadPool;
	std::vector<Entity> toDraw;

	Timer fpsTimer;
	double fpsFrameCount;

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
	VkPipeline graphicsPipeline;
	VkCommandPool singleTimeCommandsPool;
	std::vector<VkCommandPool> commandPools;
	std::vector<VkCommandPool> mainCommandPools;
	std::vector<VkCommandBuffer> entityCommandBuffers;
	std::vector<VkCommandBuffer> primCommandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	VkSampler textureSampler;
	Texture2D texture;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;

	VkDebugReportCallbackEXT callback;

	QueueFamilyIndices familyIndices;



	//compute
	int preComputedSteps = 20;
	int numEntities = 0;
	size_t mapSize;
	size_t entitiesSize;
	size_t stepsSize;
	VkDeviceSize memorySize;

	VkDeviceMemory computeMemory;
	VkBuffer map_buffer;
	VkBuffer entity_buffer;
	VkBuffer steps_buffer;

	vec2* astarSteps;

	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorSet computeDescriptorSet;
	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;
	VkDescriptorPool computeDescriptorPool;
	VkCommandPool computeCommandPool;
	VkCommandBuffer computeCommandBuffer;
	VkWriteDescriptorSet computeWriteDescriptorSet[3];

	int alignOffsetEntity = 0;
	int alignOffsetSteps = 0;
};