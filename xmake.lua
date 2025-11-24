
add_rules("mode.debug", "mode.release")

if not is_mode("debug") then
	set_policy("build.optimization.lto", true)
end

if is_plat("linux") then
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
	add_includedirs("ext/raylib/src", "/usr/include/SDL3")
	add_defines("PLATFORM_DESKTOP_SDL", "GRAPHICS_API_OPENGL_33")
	if is_mode("debug") then
		add_defines("DEBUG")
	end
end

target("main")
	set_policy("build.warning", true)
	set_warnings("all", "extra")
	set_kind("binary")
	add_files("src/*.cpp")
	add_cxflags("-std=c++20")
	if is_plat("linux") then
		add_deps("raylib_sdl")
	else
		add_links("raylib")
	end
	add_includedirs("ext/raylib/src")
	add_links("SDL3", "sqlite3", "tcl")
	set_rundir(".")
	if is_mode("debug") then
		add_defines("DEBUG")
	end

