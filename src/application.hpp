#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <thread>
#include "renderer/renderer.hpp"
#include "util/threadpool.hpp"
#include "util/timer.hpp"


class Application
{
public:
	void run();
private:
	void init();
	void update();
	void cleanup();

	Renderer renderer;

	//threads
	std::thread* tCompute;
	std::thread* tTransfer;
	std::vector<std::thread*> tDraws;
	//thread routines
	void compute();
	void draw(int idx);
	void transfer();

	int test = 0;
	int asd = 0;

	Timer timer;
	ThreadPool threadPool;
};