#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

class Application
{
public:
	void run();
private:
	void init();
	void update();
	void cleanup();

	void initWindow();

	void initVulkan();
	void createInstance();

	int width = 800;
	int height = 600;



	GLFWwindow* window;

	VkInstance instance;
	VkDebugReportCallbackEXT callback;
};