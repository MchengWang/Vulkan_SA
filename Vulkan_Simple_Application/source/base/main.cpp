#include <iostream>
#include <stdexcept>
#include <cstdlib>

import vulkan_hpp;

#include <GLFW/glfw3.h>

class Triangle
{
/**
* accessible to all
*/
public:
	void run()
	{
		initVulkan();
		mainLoop();
		cleanup();
	}

/**
* private functions (no exposed)
*/
private:
	void initVulkan()
	{

	}

	void mainLoop()
	{

	}

	void cleanup()
	{

	}

/**
* private variables (private functions calls only)
*/
private:

};

int main()
{
	try
	{
		Triangle triangle;
		triangle.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}