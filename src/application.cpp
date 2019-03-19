
#include "application.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>



void Application::run()
{
	init();
	while (!glfwWindowShouldClose(window))
	{
		update();
		glfwPollEvents();
	}
}

void Application::init()
{
	initWindow();
	renderer.init(window);
}

void Application::update()
{

}

void Application::cleanup()
{
	renderer.cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::initWindow()
{
	if (!glfwInit())
		throw std::runtime_error("failed to init glfw");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	if(!window)
		throw std::runtime_error("failed to create glfw window");
}
