
#include "application.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <utility>
#include <future>

int nDrawThreads = 4;
bool running = true;

//proposed thread solution. naive threads executed from application, record sync handled in renderer
void Application::compute() {
	while (running) {
		//renderer.recordCompute() ??
		//printf("compute\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //remove me
	}
}

void Application::draw(int idx) {
	while (running) {
		//renderer.recordDraw (...) split draw records into chunks
		//printf("draw %d\n", idx);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Application::transfer() {
	while (running) {
		//...
		//printf("transfer\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
}

void Application::init()
{
	renderer.init();

	threadPool.init(4);

	//init threads
	tCompute = new std::thread(&Application::compute, this);
	for (int i = 0; i < nDrawThreads; i++) {
		std::thread* td = new std::thread(&Application::draw, this, i);
		tDraws.push_back(td);
	}
	tTransfer = new std::thread(&Application::transfer, this);
	
	test = 4;
	/*
	//sync funcs
	tCompute->join();
	tTransfer->join();
	td->join();
	*/
}

void Application::update()
{
	if (timer.elapsed() > 2.0)
	{
		timer.restart();
	}
	std::vector<std::future<void>> futures;
	futures.reserve(4);

	Timer t;
	t.restart();

	bool mine = true;

	if (timer.elapsed() < 1)
	{
		for (int i = 0; i < 4; i++)
			threadPool.submit([] {});
		threadPool.waitForAll();
	}
	else
	{
		mine = false;
		
		for (int i = 0; i < 4; i++)
		{
			futures.push_back(std::async(std::launch::async, [] {}));
		}
		for (int i = 0; i < 4; i++)
		{
			futures[i].wait();
		}
	}
	double time = t.restart();

	if (mine)
		std::cout << "mine: ";
	else 
		std::cout << "othr: ";
	for (int i = 0; i < time*1000000.0; i++)
	{
		std::cout << "|";
	}
	std::cout << "\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(1000/144));
}

void Application::cleanup()
{
	renderer.cleanup();
}