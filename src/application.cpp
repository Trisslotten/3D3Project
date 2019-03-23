
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
	world.init("test2.png", 8);
	world.printEntities();

	//renderer.init();
	renderer.init(world.getMap(), world.getMapDims());
	renderer.allocateComputeMemory(world.mapSize, world.entitiesSize);
	renderer.mapComputeMemory(world.origMap, world.entities.data(), world.mapSize, world.entitiesSize);
	
}

void Application::update()
{

}

void Application::cleanup()
{
	renderer.cleanup();
}