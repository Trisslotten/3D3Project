
#include "application.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <utility>
#include <future>

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
	//world.init(100, 100, 10);
	world.init("test2.png", 1);
	world.printEntities();

	renderer.init();
}

void Application::update()
{

}

void Application::cleanup()
{
	renderer.cleanup();
}