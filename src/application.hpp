#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include "renderer/renderer.hpp"


class Application
{
public:
	void run();
private:
	void init();
	void update();
	void cleanup();

	void initWindow();

	int width = 800;
	int height = 600;
	GLFWwindow* window;
	Renderer renderer;
};