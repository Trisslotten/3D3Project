#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <thread>
#include "renderer/renderer.hpp"
#include "util/threadpool.hpp"
#include "util/timer.hpp"
#include "world.h"


class Application
{
public:
	void run();
private:
	void init();
	void update();
	void cleanup();

	Renderer renderer;
	World world;

	Timer timer;
};