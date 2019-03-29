#include <exception>
#include <iostream>
#include "application.hpp"

int GLOBAL_NUM_THREADS = 4;
int GLOBAL_NUM_ENTITIES = 10;

int main(void)
{
	// TODO: set num threads from program arguments
	GLOBAL_NUM_THREADS = 8;
	GLOBAL_NUM_ENTITIES = 127;

	Application app;
	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}