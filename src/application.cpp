
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
	std::string map = "test2.png";
	world.init(map, 8);
	world.printEntities();

	renderer.init(map);
	renderer.initCompute(world.mapSize, world.entitiesSize);
	renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
	renderer.executeCompute();
}

void Application::update()
{
	for (auto e : world.getEntities())
	{
		renderer.submitEntity(e);
	}
	renderer.submitEntity(Entity(world.goal.x, world.goal.y, true));

	renderer.render();
}

void Application::cleanup()
{
	renderer.cleanup();
}