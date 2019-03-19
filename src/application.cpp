
#include "application.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>



void Application::run()
{
	init();
	while (!renderer.windowShouldClose())
	{
		update();
		glfwPollEvents();
	}
}

void Application::init()
{
	renderer.init();
}

void Application::update()
{

}

void Application::cleanup()
{
	renderer.cleanup();
}