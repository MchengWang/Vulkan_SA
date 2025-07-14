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

	void createInstance()
	{
		/**
		* ʵ����Ӧ���� Vulkan ��֮������ӣ�������ʵ���漰����������ָ���й�Ӧ�õ�һЩ��ϸ��Ϣ��
		*/
		constexpr vk::ApplicationInfo appInfo{
			.pApplicationName = "Triangle",
			.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
			.pEngineName = "Engine",
			.engineVersion = VK_MAKE_VERSION( 1, 0, 0 ),
			.apiVersion = vk::ApiVersion14
		};

		// �� GLFW ��ȡ�����ʵ����չ��
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		// ��� Vulkan ʵ���Ƿ�֧������� GLFW ��չ��
		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			if (std::ranges::none_of(extensionProperties,
				[glfwExtension = glfwExtensions[i]](auto const& extensionProperty)
				{ return strcmp(extensionProperty.extensionName, glfwExtension) == 0; }))
				throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
		}

		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions
		};

		instance = vk::raii::Instance(context, createInfo);

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
	GLFWwindow* window = nullptr;

	vk::raii::Context context;
	vk::raii::Instance instance = nullptr;

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