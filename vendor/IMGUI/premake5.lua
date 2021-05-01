project "IMGUI"
	kind "StaticLib"
	language "C++"

	targetdir "bin/%{cfg.buildcfg}"
	objdir "bin-int/%{cfg.buildcfg}"

	files {
		"imgui/imgui.cpp",
		"imgui/imgui_draw.cpp",
		"imgui/imgui_tables.cpp",
		"imgui/imgui_widgets.cpp",
		"imgui/backends/imgui_impl_glfw.cpp",
 		"imgui/backends/imgui_impl_opengl3.cpp"
	}

	includedirs {
		"imgui/",
		"../GLFW/glfw/include/",
		"../GLEW/glew/include"
	}
