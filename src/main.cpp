#include <exception>
#include <iostream>
#include "application.hpp"

int GLOBAL_NUM_THREADS = 1;
int GLOBAL_NUM_ENTITIES = 250;
bool GLOBAL_TESTING = true;

int main(int argc, char *argv[])
{
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