
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
	std::string map = "test3.png";
	world.init(map, GLOBAL_NUM_ENTITIES);
	world.printEntities();

	renderer.init(map);
	renderer.initCompute(world.mapSize, world.entitiesSize);
	renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
	renderer.executeCompute();
	world.setSteps(renderer.getSteps());
}

void Application::update()
{
	if (world.getStepsCount() <= 0 || world.finished || world.getGoalReached()) {
		if (world.numComputes > 5) {
			world.setNewGoal();
		}
		renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
		renderer.executeCompute();
		world.setSteps(renderer.getSteps());
	}

	if (timer.elapsed() >= 0.02) {
		world.updateEntities();

		timer.restart();
	}

	for (auto e : world.getEntities())
	{
		renderer.submitEntity(Entity(e.x,e.y));
	}
	renderer.submitEntity(Entity(world.goal.x, world.goal.y, true));
	renderer.render();
}

void Application::cleanup()
{
	renderer.cleanup();
}