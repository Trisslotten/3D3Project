
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
	std::string map = "test.png";
	world.init(map, 1);
	world.printEntities();

	renderer.init(map);
	renderer.initCompute(world.mapSize, world.entitiesSize);
	renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
	renderer.executeCompute();
	world.setSteps(renderer.getSteps());
}

void Application::update()
{
	
	if (timer.elapsed() >= 0.03) {
		world.updateEntities();
	

		if (world.getStepsCount() <= 0 || world.finished) {
			renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
			renderer.executeCompute();
			world.setSteps(renderer.getSteps());
		}

		timer.restart();
	}

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