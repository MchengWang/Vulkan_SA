#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <algorithm>
#include <memory>

#include <string>
#include <vector>

#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif // __INTELLISENSE__

#include <vulkan/vk_platform.h>

#include <GLFW/glfw3.h>

const uint32_t WIDTH = 1290;
const uint32_t HEIGHT = 720;
const std::string TITLE = "TRIANGLE";

const std::vector validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif // NDEBUG


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
		createInstance();
		setupDebugMessenger();
		pickPhysicalDevice();
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

		// ��ȡ��Ҫ�ı�ʾ��
		std::vector<char const*> requiredLayers;
		if (enableValidationLayers)
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());

		// ��� Vulkan ʵ���Ƿ�֧������Ҫ�ı�ʾ��
		auto layerProperties = context.enumerateInstanceLayerProperties();
		if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer)
			{
				return std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty)
					{
						return strcmp(layerProperty.layerName, requiredLayer) == 0;
					});
			}))
			throw std::runtime_error("One or more required layers are not supported!");

		// �� GLFW ��ȡ�����ʵ����չ��
		auto requiredExtensions = getRequiredExtensions();

		// ��� Vulkan ʵ���Ƿ�֧������� GLFW ��չ��
		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		for (auto const& requiredExtension : requiredExtensions)
		{
			if (std::ranges::none_of(extensionProperties,
				[requiredExtension](auto const& extensionProperty)
				{ return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
				throw std::runtime_error("Required GLFW extension not supported: " + std::string(requiredExtension));
		}

		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data(),
		};

		instance = vk::raii::Instance(context, createInfo);

	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		// ָ��ϣ�����ûش����������͵�������
		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
															vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
															vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		// ɸѡ֪ͨ�ش�����Ϣ����
		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
														   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
														   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
			.messageSeverity = severityFlags,
			.messageType = messageTypeFlags,
			.pfnUserCallback = &debugCallback
		};
		debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void pickPhysicalDevice()
	{
		// �г����������Կ�
		auto devices = instance.enumeratePhysicalDevices();

		const auto devIter = std::ranges::find_if(devices, [&](auto const& device)
			{
				auto queueFamilies = device.getQueueFamilyProperties();
				bool isSuitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
				const auto qfpIter = std::ranges::find_if(queueFamilies, [](vk::QueueFamilyProperties const& qfp)
					{
						return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
					});

				isSuitable = isSuitable && (qfpIter != queueFamilies.end());
				auto extensions = device.enumerateDeviceExtensionProperties();
				bool found = true;
				for (auto const& extension : deviceExtensions)
				{
					auto extensionIter = std::ranges::find_if(extensions, [extension](auto const& ext)
						{
							return strcmp(ext.extensionName, extension) == 0;
						});
					found = found && extensionIter != extensions.end();
				}

				isSuitable = isSuitable && found;
				printf("\n");

				if (isSuitable)
					physicalDevice = device;

				return isSuitable;
			});
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
* helper functions
*/
private:
	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers)
			extensions.push_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severty, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
	{
		std::cerr << "validation layer: type " << to_string(type) << "msg" << pCallbackData->pMessage << std::endl;

		return vk::False;
	}

/**
* private variables (private functions calls only)
*/
private:
	GLFWwindow* window = nullptr;

	vk::raii::Context context;
	vk::raii::Instance instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	vk::raii::PhysicalDevice physicalDevice = nullptr;

	std::vector<const char*> deviceExtensions = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};

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