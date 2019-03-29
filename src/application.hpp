#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <thread>
#include "renderer/renderer.hpp"
#include "world.h"
#include "util/timer.hpp"
#include <mutex>


class Application
{
public:
	void run();
private:
	void init();
	void update();
	void cleanup();

	void updateAstar();

	Renderer renderer;
	World world;
	Timer timer;

	std::mutex entityMutex;
	std::thread astarComputeThread;

	bool cleaned = false;
};