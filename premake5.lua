workspace "VoxelRenderer"
	architecture "x64"
	startproject "VoxelRenderer"

	configurations { "Debug", "Release" }

include "vendor/GLEW"
include "vendor/GLFW"
include "vendor/IMGUI"

project "VoxelRenderer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "c++17"
	systemversion "latest"

	targetdir "bin/%{cfg.buildcfg}"
	objdir "bin-int/%{cfg.buildcfg}"

	files {
		"src/**.h",
		"src/**.cpp"
	}

	includedirs {
		"vendor/GLEW/glew/include",
		"vendor/GLFW/glfw/include",
		"vendor/GLM/glm/",
		"vendor/IMGUI/imgui",
		"vendor/STB"
	}

	links {
		"GLEW",
		"GLFW",
		"IMGUI"
	}

	defines "GLEW_STATIC"

	filter "system:windows"
		links "opengl32.lib"

	filter "system:linux"
		linkoptions { "-lX11", "-ldl", "-lGL", "-lpthread" }

	filter "configurations:Debug"
		defines "VOXEL_RENDERER_DEBUG"
		symbols "On"
		runtime "Debug"

	filter "configurations:Release"
		defines "VOXEL_RENDERER_RELEASE"
		optimize "On"
		runtime "Release"