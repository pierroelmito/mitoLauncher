
add_rules("mode.debug", "mode.release")

target("raylib_sdl")
	set_kind("static")
	add_files(
		"ext/raylib/src/raudio.c",
		"ext/raylib/src/rcore.c",
		"ext/raylib/src/rmodels.c",
		"ext/raylib/src/rshapes.c",
		"ext/raylib/src/rtext.c",
		"ext/raylib/src/rtextures.c",
		"ext/raylib/src/utils.c"
	)
	add_includedirs("ext/raylib/src", "/usr/include/SDL2")
	add_defines("PLATFORM_DESKTOP_SDL", "GRAPHICS_API_OPENGL_33")
	add_cxflags("-flto")
	if is_mode("debug") then
		add_defines("DEBUG")
	end

target("main")
	set_policy("build.warning", true)
	set_warnings("all", "extra")
	set_kind("binary")
	add_files("src/*.cpp")
	add_cxflags("-std=c++20", "-flto")
	add_deps("raylib_sdl")
	add_includedirs("ext/raylib/src")
	add_links("SDL2", "sqlite3", "tcl")
	set_rundir(".")
	if is_mode("debug") then
		add_defines("DEBUG")
	end

