#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include "../entity.h"
#include "../world.h"
#include "../util/Threadpool.h"
#include "../util/timer.hpp"
#include "texture2D.hpp"
#include "constantbuffer.hpp"

extern int GLOBAL_NUM_THREADS;
extern int GLOBAL_NUM_ENTITIES;

#define MAX_DRAW_ENTITIES 256
#define NUM_UNIFORM_FLOATS 3

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
		threadPool(GLOBAL_NUM_THREADS-1), 
		texture(this)
	{}

	void init(const std::string& map);
	void render();
	void cleanup();

	void submitEntity(Entity e);

	bool windowShouldClose();

	void initCompute(size_t sizeMap, size_t sizeEntites);
	void mapComputeMemory(void* map, void* entities, uvec2* dims, uvec2* goal, size_t mapSize, size_t entitySize);
	void executeCompute();

	ivec2* getSteps() {
		return astarSteps;
	}
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
	void createUniformBuffers();

	void createQueryPool();

	void calcUniformBufferAlignment();

	void getVkLimits();

	void createComputePipeline();
	void createComputeCommandPools();
	void createComputeDescriptorSets();

	void createTransferCommandBuffer();
	void transferComputeDataToDevice();
	void transferComputeDataToHost();

	void updateUniformBuffer();
	void saveBenchmarkValues();

	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkShaderModule createShaderModule(const std::string& filepath);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	int commandIndex(int threadID)
	{
		return threadID + currentFrame * GLOBAL_NUM_THREADS;
	}

	friend class Texture2D;

	const int MAX_FRAMES_IN_FLIGHT = 3;
	size_t currentFrame = 0;
	int width = 800;
	int height = 600;
	GLFWwindow* window;
	threadpool::Threadpool threadPool;
	std::vector<Entity> toDraw;
	uint32_t drawCount = 0;
	float* posBuffer;


	// benchmark values
	uint64_t numValues;
	const uint64_t MAX_BENCHMARK_VALUES = 100;
	uint32_t* benchmarkValues;

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
	VkPipeline entityGraphicsPipeline;
	VkPipeline mapGraphicsPipeline;
	VkCommandPool singleTimeCommandsPool;
	std::vector<VkCommandPool> commandPools;
	std::vector<VkCommandPool> mapCommandPools;
	std::vector<VkCommandPool> mainCommandPools;
	std::vector<VkCommandBuffer> entityCommandBuffers;
	std::vector<VkCommandBuffer> mapCommandBuffers;
	std::vector<VkCommandBuffer> primCommandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkSampler textureSampler;
	Texture2D texture;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkPhysicalDeviceProperties properties;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;

	uint32_t timestampValidBitsGraphicsQueue;
	uint32_t timestampValidBitsPresentQueue;
	std::vector<VkQueryPool> queryPools;
	uint32_t timestampToNsScaling;

	VkDebugReportCallbackEXT callback;

	QueueFamilyIndices familyIndices;

	//transfer
	VkCommandPool transferCommandPool;
	VkCommandBuffer transferCommandBuffer;

	VkFence fen_transfer;

	//compute
	int preComputedSteps = 20;
	int numEntities = 0;
	size_t mapSize;
	size_t entitiesSize;
	size_t stepsSize;
	VkDeviceSize memorySize;

	uint32_t uniformBufferAlignment;

	VkSemaphore sem_transferToDevice;
	VkSemaphore sem_computeDone;

	VkDeviceMemory computeMemory_dst;
	VkDeviceMemory computeMemory_src;
	VkBuffer map_buffer_dst;
	VkBuffer map_buffer_src;
	VkBuffer entity_buffer_dst;
	VkBuffer entity_buffer_src;
	VkBuffer steps_buffer_dst;
	VkBuffer steps_buffer_src;
	VkBuffer dimsgoal_buffer_src;
	VkBuffer dimsgoal_buffer_dst;

	ivec2* astarSteps;

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
	int alignOffsetDimsGoal = 0;
};