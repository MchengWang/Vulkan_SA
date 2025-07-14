#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <algorithm>
#include <string>

import vulkan_hpp;

#include <GLFW/glfw3.h>

const uint32_t WIDTH = 1290;
const uint32_t HEIGHT = 720;
const std::string TITLE = "TRIANGLE";

class Triangle
{
/**
* accessible to all
*/
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

/**
* private functions (no exposed)
*/
private:
	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, TITLE.c_str(), nullptr, nullptr);
	}

	void initVulkan()
	{

	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow(window);

		glfwTerminate();
	}

/**
* private variables (private functions calls only)
*/
private:
	GLFWwindow* window;

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