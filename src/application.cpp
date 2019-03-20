
#include "application.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <utility>

int nDrawThreads = 4;
bool running = true;

//proposed thread solution. naive threads executed from application, record sync handled in renderer
void Application::compute() {
	while (running) {
		//renderer.recordCompute() ??
		printf("compute\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //remove me
	}
}

void Application::draw(int idx) {
	while (running) {
		//renderer.recordDraw (...) split draw records into chunks
		printf("draw %d\n", idx);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Application::transfer() {
	while (running) {
		//...
		printf("transfer\n");
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

}

void Application::cleanup()
{
	renderer.cleanup();
}