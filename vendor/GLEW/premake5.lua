project "GLEW"
	kind "StaticLib"
	language "C"

	targetdir "bin/%{cfg.buildcfg}"
	objdir "bin-int/%{cfg.buildcfg}"

	files {
		"glew/include/GL/**.h",
		"glew/src/**.c"
	}

	includedirs {
		"glew/include"
	}

	defines "GLEW_STATIC"
