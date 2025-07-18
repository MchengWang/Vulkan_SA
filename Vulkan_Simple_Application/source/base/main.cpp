#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <limits>

#include <string>
#include <vector>
#include <array>

#include <assert.h> // assert

#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif // __INTELLISENSE__

#include <vulkan/vk_platform.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

constexpr uint32_t WIDTH = 1290;
constexpr uint32_t HEIGHT = 720;
const std::string TITLE = "TRIANGLE";
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif // NDEBUG


struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
		};
	}
};

const std::vector<Vertex> vertices = {
	{ {0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
	{ {0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }},
	{ {-0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }}
};

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

		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, TITLE.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<Triangle*>(glfwGetWindowUserPointer(window));
				app->framebufferResized = true;
			});
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
		createCommandPool();
		createVertexBuffer();
		createCommandBuffer();
		createSyncObjects();
	}

	void createInstance()
	{
		/**
		* 实例是应用与 Vulkan 库之间的连接，创建该实例涉及向驱动程序指定有关应用的一些详细信息。
		*/
		constexpr vk::ApplicationInfo appInfo{
			.pApplicationName = "Triangle",
			.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
			.pEngineName = "Engine",
			.engineVersion = VK_MAKE_VERSION( 1, 0, 0 ),
			.apiVersion = vk::ApiVersion14
		};

		// 获取需要的表示层
		std::vector<char const*> requiredLayers;
		if (enableValidationLayers)
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());

		// 检查 Vulkan 实现是否支持所需要的表示层
		auto layerProperties = context.enumerateInstanceLayerProperties();
		for (auto const& requiredLayer : requiredLayers)
		{
			if (std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty)
				{
					return strcmp(layerProperty.layerName, requiredLayer) == 0;
				}))
				throw std::runtime_error("Required layer nor supported: " + std::string(requiredLayer));
		}

		// 从 GLFW 获取所需的实例扩展。
		auto requiredExtensions = getRequiredExtensions();

		// 检查 Vulkan 实现是否支持所需的 GLFW 扩展。
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

		// 指定希望调用回传的所有类型的严重性
		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
															vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
															vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		// 筛选通知回传的消息类型
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

	void createSurface()
	{
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
			throw std::runtime_error("failed to create window surface!");

		surface = vk::raii::SurfaceKHR(instance, _surface);
	}

	void pickPhysicalDevice()
	{
		std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
		const auto devIter = std::ranges::find_if(devices, [&](auto const& device)
			{
				// 检查设备是否支持 Vulkan 1.3
				bool supprotsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

				// 检查全部队列是否能支持图像操作
				auto queueFmailies = device.getQueueFamilyProperties();
				bool supportsGraphics = std::ranges::any_of(queueFmailies, [](auto const& qfp) 
					{
						return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
					});

				// 检查所需要的设备扩展是否可用
				auto avaialbleDeviceExtensions = device.enumerateDeviceExtensionProperties();
				bool supportsAllRequiredExtensions = std::ranges::all_of(requiredDeviceExtension, [&avaialbleDeviceExtensions](auto const& requiredDeviceExtension)
					{
						return std::ranges::any_of(avaialbleDeviceExtensions, [requiredDeviceExtension](auto const& availableDeviceExtension)
							{
								return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
							});
					});

				auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
				bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
					features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

				return supprotsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
			});

		if (devIter != devices.end())
			physicalDevice = *devIter;
		else
			throw std::runtime_error("failed to find a suitable GPU!");
	}

	void createLogicalDevice()
	{
		// 指定想要的单个队列的数量
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		// 将第一个索引获取到支持图形的 queueFamilyProperties 中
		auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const& qfp)
			{
				return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
			});

		assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");

		auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

		// 确定支持 present 的 queueFamilyIndex
		// 首先检查 graphicsIndex 是否足够好
		auto presentIndex = physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface)
			? graphicsIndex
			: ~0;

		if (presentIndex == queueFamilyProperties.size())
		{
			for (size_t i = 0; i < queueFamilyProperties.size(); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
					physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
				{
					graphicsIndex = static_cast<uint32_t>(i);
					presentIndex = graphicsIndex;
					break;
				}
			}

			if (presentIndex == queueFamilyProperties.size())
			{
				for (size_t i = 0; i < queueFamilyProperties.size(); i++)
				{
					if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
					{
						presentIndex = static_cast<uint32_t>(i);
						break;
					}
				}
			}
		}

		if ((graphicsIndex == queueFamilyProperties.size()) || (presentIndex == queueFamilyProperties.size()))
			throw std::runtime_error("Could not find a queue for graphics or present -> terminating");

		// 创建一个功能结构链
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
			{}, // vk::PhysicalDeviceFeatures2 (empty for now)
			{ .synchronization2 = true, .dynamicRendering = true }, // 从 Vulkan 1.3 启用动态渲染
			{ .extendedDynamicState = true} // 从扩展启用扩展动态状态
		};

		float queuePriority = 0.0f;

		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
			.queueFamilyIndex = graphicsIndex,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};

		vk::DeviceCreateInfo deviceCreateInfo{
			.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &deviceQueueCreateInfo,
			.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
			.ppEnabledExtensionNames = requiredDeviceExtension.data()
		};

		device = vk::raii::Device(physicalDevice, deviceCreateInfo);
		graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
		presentQueue = vk::raii::Queue(device, presentIndex, 0);
	}

	void createSwapChain()
	{
		auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		swapChainImageFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
		swapChainExtent = chooseSwapExtent(surfaceCapabilities);
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ?
			surfaceCapabilities.maxImageCount : minImageCount;
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
			.surface = surface,
			.minImageCount = minImageCount,
			.imageFormat = swapChainImageFormat,
			.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
			.imageExtent = swapChainExtent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.preTransform = surfaceCapabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(surface)),
			.clipped = true
		};

		swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
		swapChainImages = swapChain.getImages();
	}

	void createImageViews()
	{
		swapChainImageViews.clear();

		vk::ImageViewCreateInfo imageViewCreateInfo{
			.viewType = vk::ImageViewType::e2D,
			.format = swapChainImageFormat,
			.subresourceRange = {
				vk::ImageAspectFlagBits::eColor,
				0, 1, 0, 1
			}
		};

		for (auto image : swapChainImages)
		{
			imageViewCreateInfo.image = image;
			swapChainImageViews.emplace_back(device, imageViewCreateInfo);
		}
	}

	void createGraphicsPipeline()
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(readFile("resources/shaders/slang_vertexbuffer.spv"));

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = shaderModule,
			.pName = "vertMain"
		};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
			.stage = vk::ShaderStageFlagBits::eFragment, 
			.module = shaderModule,
			.pName = "fragMain"
		};
		
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		
		// 顶点输入
		auto bindindDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo
		{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindindDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};

		// 输入集合
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly
		{
			.topology = vk::PrimitiveTopology::eTriangleList
		};

		// 剪刀和视图
		vk::PipelineViewportStateCreateInfo viewportState
		{
			.viewportCount = 1,
			.scissorCount = 1
		};

		// 光栅器
		vk::PipelineRasterizationStateCreateInfo rasterizer
		{
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eBack,
			.frontFace = vk::FrontFace::eClockwise,
			.depthBiasEnable = vk::False,
			.depthBiasSlopeFactor = 1.0f,
			.lineWidth = 1.0f
		};

		// 多重采样
		vk::PipelineMultisampleStateCreateInfo multisampling
		{
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable = vk::False
		};

		// 色彩混合
		vk::PipelineColorBlendAttachmentState colorBlendAttachment
		{
			.blendEnable = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA 

		};

		vk::PipelineColorBlendStateCreateInfo colorBlending
		{
			.logicOpEnable = vk::False,
			.logicOp = vk::LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment
		};

		// 动态状态
		std::vector dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

		vk::PipelineDynamicStateCreateInfo dynamicState
		{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo
		{
			.setLayoutCount = 0,
			.pushConstantRangeCount = 0
		};

		pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		// 渲染
		vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo
		{
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &swapChainImageFormat
		};

		vk::GraphicsPipelineCreateInfo pipelineInfo
		{
			.pNext = &pipelineRenderingCreateInfo,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout,
			.renderPass = nullptr
		};

		graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
	}

	void createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo
		{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 
			.queueFamilyIndex = graphicsIndex
		};

		commandPool = vk::raii::CommandPool(device, poolInfo);
	}

	void createVertexBuffer()
	{
		vk::BufferCreateInfo stagingInfo
		{
			.size = sizeof(vertices[0]) * vertices.size(),
			.usage = vk::BufferUsageFlagBits::eTransferSrc,
			.sharingMode = vk::SharingMode::eExclusive
		};
		vk::raii::Buffer stagingBuffer(device, stagingInfo);
		vk::MemoryRequirements memRequirementsStaging = stagingBuffer.getMemoryRequirements();
		vk::MemoryAllocateInfo memoryAllocateInfoStaging
		{
			.allocationSize = memRequirementsStaging.size,
			.memoryTypeIndex = findMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
		};
		vk::raii::DeviceMemory stagingBufferMemory(device, memoryAllocateInfoStaging);

		stagingBuffer.bindMemory(stagingBufferMemory, 0);
		void* dataStaging = stagingBufferMemory.mapMemory(0, stagingInfo.size);
		memcpy(dataStaging, vertices.data(), stagingInfo.size);
		stagingBufferMemory.unmapMemory();

		vk::BufferCreateInfo bufferInfo
		{
			.size = sizeof(vertices[0]) * vertices.size(),
			.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			.sharingMode = vk::SharingMode::eExclusive
		};
		
		vertexBuffer = vk::raii::Buffer(device, bufferInfo);

		vk::MemoryRequirements memRequirements = vertexBuffer.getMemoryRequirements();
		vk::MemoryAllocateInfo memoryAllocateInfo
		{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		vertexBufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);

		vertexBuffer.bindMemory(*vertexBufferMemory, 0);

		copyBuffer(stagingBuffer, vertexBuffer, stagingInfo.size);
	}

	void createCommandBuffer()
	{
		vk::CommandBufferAllocateInfo allocInfo
		{
			.commandPool = commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT
		};

		commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
	}

	void createSyncObjects()
	{
		presentCompleteSemaphores.clear();
		renderFinishedSemaphores.clear();
		inFlightFences.clear();

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
			renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			inFlightFences.emplace_back(device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
		}
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}

		device.waitIdle();
	}

	void drawFrame() {
		while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX));

		auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[semaphoreIndex], nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR) 
		{
			recreateSwapChain();
			return;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) 
			throw std::runtime_error("failed to acquire swap chain image!");
		

		device.resetFences(*inFlightFences[currentFrame]);
		commandBuffers[currentFrame].reset();
		recordCommandBuffer(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitInfo
		{ 
			.waitSemaphoreCount = 1, 
			.pWaitSemaphores = &*presentCompleteSemaphores[semaphoreIndex],
			.pWaitDstStageMask = &waitDestinationStageMask, 
			.commandBufferCount = 1,
			.pCommandBuffers = &*commandBuffers[currentFrame],
			.signalSemaphoreCount = 1, 
			.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] 
		};
		graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

		const vk::PresentInfoKHR presentInfoKHR
		{ 
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
			.swapchainCount = 1, 
			.pSwapchains = &*swapChain, 
			.pImageIndices = &imageIndex 
		};

		result = presentQueue.presentKHR(presentInfoKHR);

		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != vk::Result::eSuccess) 
			throw std::runtime_error("failed to present swap chain image!");
		
		semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphores.size();
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo
		{
			.commandPool = commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1
		};

		vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
		commandCopyBuffer.begin(vk::CommandBufferBeginInfo
			{
				.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
			});
		commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));
		commandCopyBuffer.end();
		graphicsQueue.submit(vk::SubmitInfo
			{
				.commandBufferCount = 1,
				.pCommandBuffers = &*commandCopyBuffer
			}, nullptr);
		graphicsQueue.waitIdle();
	}

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
	
	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		device.waitIdle();

		cleanupSwapChain();
		createSwapChain();
		createImageViews();
	}

	void cleanupSwapChain()
	{
		swapChainImageViews.clear();
	}
	
	void recordCommandBuffer(uint32_t imageIndex)
	{
		commandBuffers[currentFrame].begin({});

		// transition the image layout for rendering
		transition_image_layout(
			imageIndex,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eTopOfPipe,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput
		);

		// set up the color attachment
		vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f); 
		vk::RenderingAttachmentInfo attachmentInfo = {
			.imageView = swapChainImageViews[imageIndex],
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = clearColor
		};

		// set up the rendering info
		vk::RenderingInfo renderingInfo = {
			.renderArea = {
				.offset = { 0, 0 },
				.extent = swapChainExtent
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentInfo
		};

		// Begin rendering
		commandBuffers[currentFrame].beginRendering(renderingInfo);

		// Rendering commands will go here
		commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
		commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
		commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
		commandBuffers[currentFrame].draw(3, 1, 0, 0);

		// End rendering
		commandBuffers[currentFrame].endRendering();

		// transition the image layout for presentation
		transition_image_layout(
			imageIndex, 
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite, // srcAccessMask
			{}, // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
			vk::PipelineStageFlagBits2::eBottomOfPipe // dstStage
		);

		commandBuffers[currentFrame].end();
	}

	void transition_image_layout(
		uint32_t imageIndex, 
		vk::ImageLayout old_layout,
		vk::ImageLayout new_layout,
		vk::AccessFlagBits2 src_access_mask,
		vk::AccessFlagBits2 dst_access_mask,
		vk::PipelineStageFlagBits2 src_stage_mask,
		vk::PipelineStageFlagBits2 dst_stage_mask
	)
	{
		vk::ImageMemoryBarrier2 barrier = {
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = swapChainImages[imageIndex],
			.subresourceRange = 
			{
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		vk::DependencyInfo dependency_info = {
			.dependencyFlags = {},
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier
		};

		commandBuffers[currentFrame].pipelineBarrier2(dependency_info);
	}

	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const
	{
		vk::ShaderModuleCreateInfo createInfo{
			.codeSize = code.size() * sizeof(char),
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};

		vk::raii::ShaderModule shaderModule{ device, createInfo};

		return shaderModule;
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers)
			extensions.push_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	static vk::Format chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		const auto formatIt = std::ranges::find_if(availableFormats, [](const auto& format)
			{
				return format.format == vk::Format::eB8G8R8A8Srgb &&
					format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
			});
		return formatIt != availableFormats.end() ? formatIt->format : availableFormats[0].format;
	}

	static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		return std::ranges::any_of(availablePresentModes, [](const vk::PresentModeKHR value)
			{
				return vk::PresentModeKHR::eMailbox == value;
			}) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
		};
	}

	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		std::vector<char> buffer(file.tellg());

		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();

		return buffer;
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
	vk::raii::SurfaceKHR surface = nullptr;

	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device device = nullptr;
	
	vk::raii::Queue graphicsQueue = nullptr;
	vk::raii::Queue presentQueue = nullptr;

	vk::raii::SwapchainKHR swapChain = nullptr;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat = vk::Format::eUndefined;
	vk::Extent2D swapChainExtent;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;

	vk::raii::Buffer vertexBuffer = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;

	vk::raii::CommandPool commandPool = nullptr;
	uint32_t graphicsIndex = 0;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	
	uint32_t semaphoreIndex = 0;
	uint32_t currentFrame = 0;

	std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence> inFlightFences;

	bool framebufferResized = false;

	std::vector<const char*> requiredDeviceExtension = {
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