workspace "Vulkan3DRenderer"
	architecture "x64"
	configurations {"Debug", "Release"}
	startproject "VulkanRenderer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["glm"] = "VulkanRenderer/vendor/glm"
IncludeDir["SDL2"] = "VulkanRenderer/vendor/SDL2"
IncludeDir["ImGui"] = "VulkanRenderer/vendor/ImGui"
IncludeDir["vulkan"] = "VulkanRenderer/vendor/vulkan"
include "VulkanRenderer/vendor/ImGui"

project "VulkanRenderer"
	location "VulkanRenderer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "on"

	targetdir  ("bin/" .. outputdir .. "/%{prj.name}")
	objdir  ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{IncludeDir.glm}/**.hpp",
		"%{IncludeDir.glm}/**.inl",
		"%{IncludeDir.vulkan}/**hpp",
		"%{IncludeDir.vulkan}/**h",
		"%{IncludeDir.SDL2}/**h",
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.SDL2}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.vulkan}",
	}

	links
	{
		"vendor/bin/SDL2/SDL2.lib",
		"vendor/bin/SDL2/SDL2.dll",
		"$(VULKAN_SDK)/Lib/vulkan-1.lib",
		"ImGui"
	}

	defines { "_CRT_SECURE_NO_WARNINGS" }

	filter "system:windows"
		systemversion "latest"
		
		defines
		{
			"VL_PLATFORM_WINDOWS",		
		}
		
	filter "configurations:Debug"
		defines "VL_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "VL_RELEASE"
		runtime "Release"
		optimize "on"