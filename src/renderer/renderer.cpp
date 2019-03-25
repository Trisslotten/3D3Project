#include "renderer.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <stdint.h>
#include <array>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define ALLOC(fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); fn(__VA_ARGS__, &count, vec.data()); }

VkResult CreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}
void DestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT callback,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
		func(instance, callback, pAllocator);
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	std::cerr << "VL: " << msg << "\n" << std::endl;
	return VK_FALSE;
}
const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


void Renderer::init(const std::string& map)
{
	createWindow();
	createInstance();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createFramebuffers();
	createCommandPools();
	createDescriptorSetLayout();
	createDescriptorPool();
	createSampler();
	texture.loadFromFile(map);
	createDescriptorSets();
	createGraphicsPipeline(); 
	createCommandBuffers();
	createSyncObjects();
}

void Renderer::render()
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	for (int i = 0; i < GLOBAL_NUM_THREADS; i++)
	{
		threadPool.queueTask([this, i]
		{
			int index = commandIndex(i);
			vkResetCommandPool(device, commandPools[index], 0);

			VkCommandBuffer cmdBuffer = entityCommandBuffers[index];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

			VkCommandBufferInheritanceInfo inheritanceInfo = {};
			inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.renderPass = renderPass;
			inheritanceInfo.framebuffer = VK_NULL_HANDLE;
			inheritanceInfo.occlusionQueryEnable = VK_FALSE;
			beginInfo.pInheritanceInfo = &inheritanceInfo;

			if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("failed to begin recording command buffer!");

			{
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
				for(int i = 0; i < 1; i++)
				{
					vkCmdDraw(cmdBuffer, 4, 1, 0, 0);
				}
			}
			if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
				throw std::runtime_error("failed to record command buffer!");
		});
	}

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	vkResetCommandPool(device, mainCommandPools[currentFrame], 0);

	VkCommandBuffer currentCommandBuffer = primCommandBuffers[currentFrame];

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;

	VkClearValue clearColor = { 0.f,0.f,0.f,1.f };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	std::vector<VkCommandBuffer> secondarybuffers;
	for (int i = 0; i < GLOBAL_NUM_THREADS; i++)
		secondarybuffers.push_back(entityCommandBuffers[commandIndex(i)]);

	threadPool.waitForTasks();
	vkCmdExecuteCommands(currentCommandBuffer, secondarybuffers.size(), secondarybuffers.data());

	vkCmdEndRenderPass(currentCommandBuffer);

	if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentCommandBuffer;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(presentQueue, &presentInfo);

	if (fpsTimer.elapsed() > 1.0)
	{
		double time = fpsTimer.restart();
		std::string fps = "Vulkan | FPS: " + std::to_string(fpsFrameCount/time);
		glfwSetWindowTitle(window, fps.c_str());
		fpsFrameCount = 0;
	}
	fpsFrameCount++;

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::submitEntity(Entity e)
{
	toDraw.push_back(e);
}

bool Renderer::windowShouldClose()
{
	return glfwWindowShouldClose(window);
}

void Renderer::createWindow()
{
	if (!glfwInit())
		throw std::runtime_error("failed to init glfw");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	if (!window)
		throw std::runtime_error("failed to create glfw window");
}

void Renderer::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "3D3Project";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}


	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create instance!");

	if (enableValidationLayers)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo2 = {};
		createInfo2.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo2.flags = (0
							 | VK_DEBUG_REPORT_ERROR_BIT_EXT
							 | VK_DEBUG_REPORT_WARNING_BIT_EXT
							 // | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
							 // | VK_DEBUG_REPORT_DEBUG_BIT_EXT
							 // | VK_DEBUG_REPORT_INFORMATION_BIT_EXT
							);
		createInfo2.pfnCallback = debugCallback;
		if (CreateDebugReportCallbackEXT(instance, &createInfo2, nullptr, &callback) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug report!");
		}
	}
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		std::cout << "available extensions:" << std::endl;

		for (const auto& extension : extensions)
		{
			std::cout << "\t" << extension.extensionName << std::endl;
		}
	}
}

void Renderer::createSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void Renderer::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find a suitable GPU!");

	
}

void Renderer::createLogicalDevice()
{
	familyIndices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { 
		familyIndices.graphicsFamily, 
		familyIndices.presentFamily,
		familyIndices.computeFamily,
		familyIndices.transferFamily
	};

	float queuePriority[] = 
	{
		1.f,
		1.f,
		1.f,
		1.f
	};

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = familyIndices.graphicsFamily;
	queueCreateInfo.queueCount = 4;
	queueCreateInfo.pQueuePriorities = queuePriority;
	queueCreateInfos.push_back(queueCreateInfo);

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	createInfo.enabledLayerCount = validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(device, familyIndices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, familyIndices.presentFamily,  1, &presentQueue);
	vkGetDeviceQueue(device, familyIndices.computeFamily,  2, &computeQueue);
	vkGetDeviceQueue(device, familyIndices.transferFamily, 3, &transferQueue);

	
	std::cout << "Q: " << graphicsQueue << "\n";
	std::cout << "Q: " << presentQueue << "\n";
	std::cout << "Q: " << computeQueue << "\n";
	std::cout << "Q: " << transferQueue << "\n";
	
}

void Renderer::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	std::cout << "minImageCount " << swapChainSupport.capabilities.minImageCount << "\n";
	std::cout << "maxImageCount " << swapChainSupport.capabilities.maxImageCount << "\n";
	uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		imageCount = swapChainSupport.capabilities.maxImageCount;
	if (imageCount < swapChainSupport.capabilities.minImageCount)
		imageCount = swapChainSupport.capabilities.minImageCount;


	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {
		(uint32_t)familyIndices.graphicsFamily, (uint32_t)familyIndices.presentFamily
	};

	if (familyIndices.graphicsFamily != familyIndices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	VkSwapchainKHR oldSwapChain = swapChain;
	createInfo.oldSwapchain = oldSwapChain;

	VkSwapchainKHR newSwapChain;
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain!");

	swapChain = newSwapChain;


	ALLOC(vkGetSwapchainImagesKHR, swapChainImages, device, swapChain);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Renderer::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];	// <---- IMAGES FROM SWAPCHAIN
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat; // <---- FORMAT FROM SWAPCHAIN
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create image views!");
	}
}

void Renderer::createRenderPass()
{
	// each image in the renderPass is described by an AttachmentDescription.
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// multisampling disabled
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// clear at begining of RenderPass (RP)
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// keep content at end of RP
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// used if the attachment is combined
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// used if the attachment is combined
	// layout expected when the RP begins
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// layout the attachment should have when the RP ends
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// layout compatible with PRESENT_SRC

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;


	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void Renderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

void Renderer::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Renderer::createDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes(1);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Renderer::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapChainImages.size());
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture.textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 1;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::createTransferCommandBuffer() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
	  VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	  0,
	  0,
	  familyIndices.transferFamily
	};
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vkCreateCommandPool(device, &commandPoolCreateInfo, 0, &transferCommandPool);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	  0,
	  transferCommandPool,
	  VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	  1
	};

	VkResult res = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &transferCommandBuffer);

	if (res != VK_SUCCESS) {
		printf("Failed to allocate transfer command buffer.\n");
	}
}

void Renderer::transferComputeDataToHost() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkFence fences = { fen_transfer };

	vkWaitForFences(device, 1, &fences, true, 10000000);
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = stepsSize;
	vkCmdCopyBuffer(transferCommandBuffer, steps_buffer_dst, steps_buffer_src, 1, &copyRegion);

	vkEndCommandBuffer(transferCommandBuffer);

	VkPipelineStageFlags stageFlags = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	submitInfo.pWaitSemaphores = &sem_computeDone;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &stageFlags;

	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(transferQueue);
	void* data;
	vkMapMemory(device, computeMemory_src, 0, memorySize, 0, &data);
	memcpy(astarSteps, (void*)((uintptr_t)data + mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps), stepsSize);
	vkUnmapMemory(device, computeMemory_src);

	for (int i = 0; i < numEntities; i++) {
		//printf("entity %d is at: %d %d \n", i, astarSteps[i*preComputedSteps].x, astarSteps[i*preComputedSteps].y);
		
	}
	for (int j = 0; j < preComputedSteps; j++) {
		printf("move #%d: (%d,%d) \n", j, astarSteps[0*preComputedSteps + j].x, astarSteps[0*preComputedSteps + j].y);
	}
	//printf("goal position: %d %d \n", astarSteps[2].x, astarSteps[2].y);
}

void Renderer::transferComputeDataToDevice() {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
	
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = mapSize;
	vkCmdCopyBuffer(transferCommandBuffer, map_buffer_src, map_buffer_dst, 1, &copyRegion);

	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = entitiesSize;
	vkCmdCopyBuffer(transferCommandBuffer, entity_buffer_src, entity_buffer_dst, 1, &copyRegion);

	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = sizeof(vec2)*2;
	vkCmdCopyBuffer(transferCommandBuffer, dimsgoal_buffer_src, dimsgoal_buffer_dst, 1, &copyRegion);

	vkEndCommandBuffer(transferCommandBuffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	submitInfo.pSignalSemaphores = &sem_transferToDevice;
	submitInfo.signalSemaphoreCount = 1;

	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	//vkQueueWaitIdle(transferQueue);
}

void Renderer::mapComputeMemory(void* map, void* entities, vec2* dims, vec2* goal, size_t mapSize, size_t entitiesSize)
{
	void *payload;
	VkResult res = vkMapMemory(device, computeMemory_src, 0, mapSize + entitiesSize, 0, &payload);
	memcpy(payload, map, mapSize);
	memcpy((void*)((uintptr_t)payload + mapSize + alignOffsetEntity), entities, entitiesSize);
	memcpy((void*)((uintptr_t)payload + mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps + stepsSize + alignOffsetDimsGoal), dims, sizeof(vec2));
	memcpy((void*)((uintptr_t)payload + mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps + stepsSize + alignOffsetDimsGoal + sizeof(vec2)), goal, sizeof(vec2));
	vkUnmapMemory(device, computeMemory_src);
	
	res = vkBindBufferMemory(device, map_buffer_src, computeMemory_src, 0);
	res = vkBindBufferMemory(device, entity_buffer_src, computeMemory_src,	 mapSize + alignOffsetEntity);
	res = vkBindBufferMemory(device, steps_buffer_src, computeMemory_src,	 mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps);
	res = vkBindBufferMemory(device, dimsgoal_buffer_src, computeMemory_src,		 mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps + stepsSize + alignOffsetDimsGoal);

	res = vkBindBufferMemory(device, map_buffer_dst, computeMemory_dst, 0);
	res = vkBindBufferMemory(device, entity_buffer_dst, computeMemory_dst,	mapSize + alignOffsetEntity);
	res = vkBindBufferMemory(device, steps_buffer_dst, computeMemory_dst,	mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps);
	res = vkBindBufferMemory(device, dimsgoal_buffer_dst, computeMemory_dst,		mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps + stepsSize + alignOffsetDimsGoal);

	//transferComputeData();

	VkDescriptorBufferInfo map_descriptorBufferInfo = {
	  map_buffer_dst,
	  0,
	  VK_WHOLE_SIZE
	};

	VkDescriptorBufferInfo entity_descriptorBufferInfo = {
	  entity_buffer_dst,
	  0,
	  VK_WHOLE_SIZE
	};

	VkDescriptorBufferInfo steps_descriptorBufferInfo = {
	  steps_buffer_dst,
	  0,
	  VK_WHOLE_SIZE
	};

	VkDescriptorBufferInfo dimsgoal_descriptorBufferInfo = {
	  dimsgoal_buffer_dst,
	  0,
	  VK_WHOLE_SIZE
	};

	VkWriteDescriptorSet computeWriteDescriptorSet[4] = {
		  {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			0,
			computeDescriptorSet,
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			0,
			&map_descriptorBufferInfo,
			0
		  },
		  {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			0,
			computeDescriptorSet,
			1,
			0,
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			0,
			&entity_descriptorBufferInfo,
			0
		  },
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			0,
			computeDescriptorSet,
			2,
			0,
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			0,
			&steps_descriptorBufferInfo,
			0
		},
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			0,
			computeDescriptorSet,
			3,
			0,
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			0,
			&dimsgoal_descriptorBufferInfo,
			0
		}
	};
	vkUpdateDescriptorSets(device, 4, computeWriteDescriptorSet, 0, 0);

	if (res == VK_SUCCESS) {
		printf("Mapped compute memory successfully!\n");
	}
	else {
		printf("Failed to map compute memory!\n");
	}
}

void Renderer::executeCompute() {
	transferComputeDataToDevice();

	VkCommandBufferBeginInfo commandBufferBeginInfo = {
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	  0,
	  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	  0
	};

	vkBeginCommandBuffer(computeCommandBuffer, &commandBufferBeginInfo);

	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		computePipelineLayout, 0, 1, &computeDescriptorSet, 0, 0);

	vkCmdDispatch(computeCommandBuffer, numEntities, 1, 1);

	vkEndCommandBuffer(computeCommandBuffer);

	//vkGetDeviceQueue(device, familyIndices.computeFamily, 0, &computeQueue);

	VkSubmitInfo submitInfo = {
	  VK_STRUCTURE_TYPE_SUBMIT_INFO,
	  0,
	  0,
	  0,
	  0,
	  1,
	  &computeCommandBuffer,
	  0,
	  0
	};

	//wait for transferToDevice, signal computeDone
	VkPipelineStageFlags stageFlags = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	submitInfo.pWaitSemaphores = &sem_transferToDevice;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &stageFlags;
	submitInfo.pSignalSemaphores = &sem_computeDone;
	submitInfo.signalSemaphoreCount = 1;

	vkQueueSubmit(computeQueue, 1, &submitInfo, fen_transfer);

	//vkQueueWaitIdle(computeQueue);
	transferComputeDataToHost();
}

void Renderer::initCompute(size_t sizeMap, size_t sizeEntites)
{
	mapSize = sizeMap;
	entitiesSize = sizeEntites;

	numEntities = sizeEntites / sizeof(Entity);
	int stepLen = numEntities * preComputedSteps;
	astarSteps = new vec2[stepLen];
	stepsSize = stepLen * sizeof(vec2);

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = sizeMap;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.pQueueFamilyIndices = (uint32_t*)(&familyIndices.computeFamily);
	bufferCreateInfo.queueFamilyIndexCount = 1;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.pNext = 0;

	if (vkCreateBuffer(device, &bufferCreateInfo, 0, &map_buffer_src) != VK_SUCCESS) {
		printf("failed to create buffer!\n");
	}
	vkCreateBuffer(device, &bufferCreateInfo, 0, &map_buffer_dst);

	bufferCreateInfo.size = sizeEntites;
	vkCreateBuffer(device, &bufferCreateInfo, 0, &entity_buffer_src);
	vkCreateBuffer(device, &bufferCreateInfo, 0, &entity_buffer_dst);

	bufferCreateInfo.size = stepsSize;
	vkCreateBuffer(device, &bufferCreateInfo, 0, &steps_buffer_src);
	vkCreateBuffer(device, &bufferCreateInfo, 0, &steps_buffer_dst);

	bufferCreateInfo.size = 2 * sizeof(vec2);
	vkCreateBuffer(device, &bufferCreateInfo, 0, &dimsgoal_buffer_dst);
	vkCreateBuffer(device, &bufferCreateInfo, 0, &dimsgoal_buffer_src);

	VkMemoryRequirements reqs;
	
	vkGetBufferMemoryRequirements(device, entity_buffer_src, &reqs);
	vkGetBufferMemoryRequirements(device, entity_buffer_dst, &reqs);
	vkGetBufferMemoryRequirements(device, entity_buffer_dst, &reqs);
	vkGetBufferMemoryRequirements(device, steps_buffer_src, &reqs);
	vkGetBufferMemoryRequirements(device, map_buffer_src, &reqs);
	vkGetBufferMemoryRequirements(device, map_buffer_dst, &reqs);
	alignOffsetEntity = reqs.alignment - (sizeMap % reqs.alignment);
	vkGetBufferMemoryRequirements(device, steps_buffer_dst, &reqs);
	alignOffsetSteps = reqs.alignment - ((sizeMap+alignOffsetEntity+sizeEntites) % reqs.alignment);
	
	vkGetBufferMemoryRequirements(device, dimsgoal_buffer_dst, &reqs);
	vkGetBufferMemoryRequirements(device, dimsgoal_buffer_src, &reqs);
											//mapSize + alignOffsetEntity + entitiesSize + alignOffsetSteps + stepsSize
	alignOffsetDimsGoal = reqs.alignment - ((sizeMap + alignOffsetEntity + sizeEntites + alignOffsetSteps + stepsSize) % reqs.alignment);

	VkPhysicalDeviceMemoryProperties properties;

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);



	memorySize = sizeMap + sizeEntites + stepLen * sizeof(vec2) + 2 * sizeof(vec2) + alignOffsetEntity + alignOffsetSteps + alignOffsetDimsGoal + reqs.size;

	// set memoryTypeIndex to an invalid entry in the properties.memoryTypes array
	uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;

	for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
		if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & properties.memoryTypes[k].propertyFlags) &&
			(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & properties.memoryTypes[k].propertyFlags) &&
			(memorySize < properties.memoryHeaps[properties.memoryTypes[k].heapIndex].size)) {
			memoryTypeIndex = k;
			break;
		}
	}

	const VkMemoryAllocateInfo memoryAllocateInfo = {
	  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	  0,
	  memorySize,
	  memoryTypeIndex
	};

	if (vkAllocateMemory(device, &memoryAllocateInfo, 0, &computeMemory_dst) == VK_SUCCESS) {
		printf("Allocated compute destination memory!\n");
	}
	else {
		printf("Failed to allocate compute memory!\n");
	}
	
	if (vkAllocateMemory(device, &memoryAllocateInfo, 0, &computeMemory_src) == VK_SUCCESS) {
		printf("Allocated compute source memory!\n");
	}
	else {
		printf("Failed to allocate compute memory!\n");
	}

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[4] = {
	  {
		0,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0
	  },
	  {
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0
	  },
	  {
		2,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0
	  },
	  {
		3,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0
	  }
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
	  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	  0,
	  0,
	  4,
	  descriptorSetLayoutBindings
	};

	if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, 0, &computeDescriptorSetLayout) != VK_SUCCESS) {
		printf("Failed to create compute descriptor set layout!\n");
	}

	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.pNext = NULL;

	vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &sem_transferToDevice);
	vkCreateSemaphore(device, &semaphoreCreateInfo, 0, &sem_computeDone);

	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	fenceInfo.pNext = NULL;
	vkCreateFence(device, &fenceInfo, 0, &fen_transfer);

	createComputePipeline();
	createComputeDescriptorSets();
	createComputeCommandPools();
	createTransferCommandBuffer();
}

void Renderer::createComputePipeline() {
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
	  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	  0,
	  0,
	  1,
	  &computeDescriptorSetLayout,
	  0,
	  0
	};
	vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, 0, &computePipelineLayout);

	VkShaderModule shader_module = createShaderModule("shader.comp");

	VkComputePipelineCreateInfo computePipelineCreateInfo = {
	  VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
	  0,
	  0,
	  {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		0,
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		shader_module,
		"main",
		0
	  },
	  computePipelineLayout,
	  0,
	  0
	};

	vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, 0, &computePipeline);
}

void Renderer::createComputeDescriptorSets() {
	VkDescriptorPoolSize descriptorPoolSize = {
	  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	  3
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	  VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	  0,
	  0,
	  1,
	  1,
	  &descriptorPoolSize
	};

	vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, 0, &computeDescriptorPool);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
	  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	  0,
	  computeDescriptorPool,
	  1,
	  &computeDescriptorSetLayout
	};

	vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &computeDescriptorSet);
}

void Renderer::createComputeCommandPools() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
	  VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	  0,
	  0,
	  familyIndices.computeFamily
	};

	vkCreateCommandPool(device, &commandPoolCreateInfo, 0, &computeCommandPool);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
	  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	  0,
	  computeCommandPool,
	  VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	  1
	};

	vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &computeCommandBuffer);
}

void Renderer::createGraphicsPipeline()
{
	VkShaderModule vertShaderModule = createShaderModule("shader.vert");
	VkShaderModule fragShaderModule = createShaderModule("shader.frag");


	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; 
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Renderer::createCommandPools()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	{
		int numCommandPools = GLOBAL_NUM_THREADS * MAX_FRAMES_IN_FLIGHT;
		commandPools.resize(numCommandPools);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0; // Optional
		for (int i = 0; i < numCommandPools; i++)
		{
			if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPools[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create a entity command pool!");
			}
		}
	}

	{
		mainCommandPools.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0; // Optional
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateCommandPool(device, &poolInfo, nullptr, &mainCommandPools[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create main command pool!");
			}
		}
	}
	
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0; // Optional
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &singleTimeCommandsPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create singleTimeCommandsPool!");
		}
	}
}

void Renderer::createCommandBuffers()
{
	{
		int numSecCmdBuffers = GLOBAL_NUM_THREADS * MAX_FRAMES_IN_FLIGHT;
		entityCommandBuffers.resize(numSecCmdBuffers);
		for (int i = 0; i < numSecCmdBuffers; i++)
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = commandPools[i];
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = 1;

			if (vkAllocateCommandBuffers(device, &allocInfo, &entityCommandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate command buffers!");
		}
	}


	{

		primCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			// use first since not done concurrently
			allocInfo.commandPool = mainCommandPools[i];
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			if (vkAllocateCommandBuffers(device, &allocInfo, &primCommandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate command buffers!");
		}
	}
}

void Renderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if ((vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) |
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) |
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i])) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void Renderer::createSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);
	return indices.isComplete();
}

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0)
		{
			if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;
			if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
				indices.computeFamily = i;
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
				indices.transferFamily = i;
			if (presentSupport)
				indices.presentFamily = i;
		}
		if (indices.isComplete())
			break;
		i++;
	}

	return indices;
}

SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	ALLOC(vkGetPhysicalDeviceSurfaceFormatsKHR, details.formats, device, surface);
	ALLOC(vkGetPhysicalDeviceSurfacePresentModesKHR, details.presentModes, device, surface);
	return details;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	// https://vulkan.lunarg.com/doc/view/1.0.26.0/linux/vkspec.chunked/ch29s05.html
	if (capabilities.currentExtent.width != 0xFFFFFFFF)
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = glm::max(capabilities.minImageExtent.width, glm::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = glm::max(capabilities.minImageExtent.height, glm::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

static std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file: " + filename + "'");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule Renderer::createShaderModule(const std::string& filepath)
{
	std::string compiledFilename = filepath + ".spv";
	remove(compiledFilename.c_str());

	std::string command = "glslangValidator.exe -V " + filepath + " -S ";

	std::string extension = filepath.substr(filepath.size() - 4, 4);
	command += extension;
	command += " -o " + compiledFilename;

	system(command.c_str());

	auto spv = readFile(compiledFilename);
	if (spv.size() == 0)
	{
		throw std::runtime_error("Could not compile shader: '" + filepath + "'");
	}
	remove(compiledFilename.c_str());

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spv.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(spv.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

VkCommandBuffer Renderer::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = singleTimeCommandsPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	vkFreeCommandBuffers(device, singleTimeCommandsPool, 1, &commandBuffer);
}

