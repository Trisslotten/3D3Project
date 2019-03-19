#include "renderer.hpp"
#include <iostream>
#include <vector>
#include <set>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


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


void Renderer::init(GLFWwindow* window)
{
	createInstance();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
}

void Renderer::cleanup()
{
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
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
							 //| VK_DEBUG_REPORT_INFORMATION_BIT_EXT
							 | VK_DEBUG_REPORT_WARNING_BIT_EXT
							 //| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
							 | VK_DEBUG_REPORT_ERROR_BIT_EXT
							 //| VK_DEBUG_REPORT_DEBUG_BIT_EXT
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

void Renderer::createSurface(GLFWwindow* window)
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
		familyIndices.computeFamily
	};

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	//deviceFeatures.fillModeNonSolid = VK_TRUE;

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
	vkGetDeviceQueue(device, familyIndices.presentFamily, 0, &presentQueue);
	vkGetDeviceQueue(device, familyIndices.computeFamily, 0, &computeQueue);
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
			if (presentSupport)
				indices.presentFamily = i;
		}

		if (indices.isComplete())
			break;
		i++;
	}

	return indices;
}
