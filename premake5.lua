workspace "Vulkan_SA"
	architecture "x64"

	configurations
	{
		"DebugX64",
		"ReleaseX64"
	}

Vulkan_SDK = os.getenv("VULKAN_SDK")

outdir = "%{cfg.buildcfg}"

third_party_path = "%{prj.name}/third_party"

IncludeDir = {}
IncludeDir["GLFW"] = third_party_path .. "/glfw"
IncludeDir["STB_IMAGE"] = third_party_path .. "/stb_image"
IncludeDir["TINY_OBJ_LOADER"] = third_party_path .. "/tinyobjloader"

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

		third_party_path .. "/glm/**.hpp",
		third_party_path .. "/glm/**.inl",
		"%{IncludeDir.STB_IMAGE}/*.h",
		"%{IncludeDir.TINY_OBJ_LOADER}/*.h",

		"%{Vulkan_SDK}/Include/vulkan/vulkan.cppm",
	}

	includedirs
	{
		"%{prj.name}/source",
		"%{prj.name}/third_party",

		"%{IncludeDir.GLFW}/include",
		"%{IncludeDir.STB_IMAGE}",
		"%{IncludeDir.TINY_OBJ_LOADER}",
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
		"STB_IMAGE_IMPLEMENTATION",
		"TINYOBJLOADER_IMPLEMENTATION",
	}

	filter "configurations:DebugX64"
		defines "VSA_Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:ReleaseX64"
		defines "VSA_Release"
		runtime "Release"
		optimize "On"