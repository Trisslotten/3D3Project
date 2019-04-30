
#include "application.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <utility>
#include <future>
#include <thread>

extern int GLOBAL_NUM_ENTITIES;

void Application::updateAstar() {
	while (!cleaned) {
		if (world.getStepsCount() <= 0 || world.finished || world.getGoalReached()) {
			if (world.numComputes > 5) {
				world.setNewGoal();
			}
			renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
			renderer.executeCompute();
			world.setSteps(renderer.getSteps());
		}

		if (timer.elapsed() >= 0.001) {
			std::lock_guard<std::mutex> lock(entityMutex);
			world.updateEntities();
			timer.restart();
		}
	}
}


void Application::run()
{
	init();
	while (!renderer.windowShouldClose())
	{
		update();
		glfwPollEvents();
	}
	// hack
	exit(0);
}

void Application::init()
{
	//world.init(100, 100, 10);
	std::string map = "test3.png";
	world.init(map, GLOBAL_NUM_ENTITIES);
	//world.printEntities();

	renderer.init(map);
	renderer.initCompute(world.mapSize, world.entitiesSize);
	
	renderer.mapComputeMemory(world.origMap, world.entities.data(), &world.getMapDims(), &world.goal, world.mapSize, world.entitiesSize);
	renderer.executeCompute();
	world.setSteps(renderer.getSteps());
	astarComputeThread = std::thread(&Application::updateAstar, this);
}

void Application::update()
{
	/*if (world.getStepsCount() <= 0 || world.finished || world.getGoalReached()) {
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
	}*/
	

	std::vector<uvec2> entities;
	{
		std::lock_guard<std::mutex> lock(entityMutex);
		entities = world.getEntities();
	}
	for (auto e : entities)
	{
		renderer.submitEntity(Entity(e.x,e.y));
	}


	renderer.submitEntity(Entity(world.goal.x, world.goal.y, true));
	renderer.render();
}

void Application::cleanup()
{
	renderer.cleanup();
	cleaned = true;
	astarComputeThread.detach();
}