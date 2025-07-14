workspace "Vulkan_SA"
	architecture "x64"

	configurations
	{
		"DebugX64",
		"ReleaseX64"
	}

Vulkan_SDK = os.getenv("VULKAN_SDK")

outdir = "%{cfg.buildcfg}"

IncludeDir = {}
IncludeDir["GLFW"] = "%{prj.name}/third_party/glfw"

project "Vulkan_Simple_Application"
	kind "ConsoleApp"
	location "Vulkan_Simple_Application"
	cppdialect "C++20"

	targetdir("binaries/" .. outdir)
	objdir("intermediaries/" .. outdir)

	files
	{
		"%{prj.name}/source/**.cpp",
		"%{prj.name}/source/**.h",
		"%{prj.name}/third_party/glm/**.hpp",
		"%{prj.name}/third_party/glm/**.inl",
		"%{Vulkan_SDK}/Include/vulkan/vulkan.cppm",
	}

	includedirs
	{
		"%{prj.name}/source",
		"%{prj.name}/third_party",

		"%{IncludeDir.GLFW}/include",
		"%{Vulkan_SDK}/Include",
	}

	links
	{
		"glfw3.lib",
		"vulkan-1.lib",
	}

	libdirs
	{
		"%{IncludeDir.GLFW}/lib-vc2022",
		"%{Vulkan_SDK}/Lib",
	}

	defines
	{
		"GLFW_INCLUDE_VULKAN",
		"VULKAN_HPP_NO_STRUCT_CONSTRUCTORS",
	}

	filter "configurations:DebugX64"
		defines "VSA_Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:ReleaseX64"
		defines "VSA_Release"
		runtime "Release"
		optimize "On"