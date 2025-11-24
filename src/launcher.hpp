#pragma once

#include <cstdint>

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <vector>

#include <raylib.h>
#include <sqlite3.h>
#include <tcl.h>

#define APP_NAME "mlauncher"

struct Str {
	constexpr static const char* History = "History";
	constexpr static const char* Favorites = "Favorites";
	constexpr static const char* Themes = "Themes";
};

enum class Mode {
	Clamp,
	Wrap,
};

enum class State {
	Platforms,
	Content,
	Themes,
};

enum class ItemType {
	Platform,
	View,
	Content,
	ContentMix,
	Theme,
};

struct Action {
	enum Type {
		Back,
		Activate,
		Toggle,
		Up,
		Down,
		PageUp,
		PageDown,
		Count,
	};
	static std::optional<Type> fromName(std::string_view n)
	{
		const char* names[] = {
			"Back",
			"Activate",
			"Toggle",
			"Up",
			"Down",
			"PageUp",
			"PageDown",
		};
		const auto it = std::find_if(std::begin(names), std::end(names), [&](const char* v) -> bool {
			return n == v;
		});
		if (it == std::end(names))
			return std::nullopt;
		return (Type)std::distance(std::begin(names), it);
	}
};

struct ViewState {
	size_t selected { 0 };
	size_t viewStart { 0 };
};

struct ViewItem {
	ItemType it {};
	size_t index {};
	std::string label {};
	int textWidth {};
	inline void dirty()
	{
		label.clear();
		textWidth = 0;
	}
};

struct SelectVector {
	State state { State::Platforms };
	std::string title {};
	ViewState vs {};
	std::vector<ViewItem> values {};
	template <typename U>
	U& current(std::vector<U>& data)
	{
		return data[values[vs.selected].index];
	}
};

struct Content {
	int id {};
	std::string name {};
	std::string path {};
	std::string platform {};
	std::optional<std::string> image {};
	bool favorite { false };
	std::optional<time_t> lastPlayed {};
	Texture tex {};
};

struct DirectoryInfo {
	std::string dir {};
	std::optional<std::string> imagePattern {};
	std::set<std::string> ext {};
};

struct PlatformInfo {
	std::string label {};
	std::string commandLine {};
	std::vector<DirectoryInfo> dirs {};
};

struct ColorType {
	enum Type {
		Bg,
		Border,
		Mid,
		Fg,
		Count,
	};
};

struct FontInfo {
	int textSize { 30 };
	std::string fontPath {};
	bool operator!=(const FontInfo& o) const
	{
		return textSize != o.textSize || fontPath != o.fontPath;
	}
};

struct Theme {
	std::array<Color, ColorType::Count> colors = {
		GetColor(0x000000ff),
		GetColor(0x1f1f1fff),
		GetColor(0x7f7f7fff),
		GetColor(0xffffffff),
	};
	FontInfo fi {};
	int rowSize { 35 };
	inline Color get(bool n, ColorType::Type c) const
	{
		return colors[n ? 3 - c : c];
	}
};

struct Mapping {
	std::array<std::optional<KeyboardKey>, Action::Count> keys {};
};

struct Context {
	Tcl_Interp* interp {};
	sqlite3* db {};
	uint32_t frame {};
	bool nm {};
	std::map<std::string, Theme> themes {};
	Theme theme {};
	FontInfo currentFont { 0, {} };
	Font fnt {};

	std::map<std::string, KeyboardKey> keyNameToId;
	Mapping mapping {};

	std::map<std::string, ViewState> lastSelected {};
	std::map<std::string, PlatformInfo> platformInfo {};

	std::vector<std::tuple<std::string, int>> customViews {};
	std::vector<std::tuple<std::string>> themeList {};
	std::vector<std::tuple<std::string, int>> platforms {};
	std::vector<Content> content {};

	std::vector<SelectVector> views {};
};

bool Init(Context& ctx, std::span<char*> args);
void Release(Context& ctx);
void RefreshContent(Context& ctx);
void RefreshMain(Context& ctx);

namespace Update {
struct UpdateParams {
	const float dt {};
};
bool Do(Context& ctx, const UpdateParams p);
}

namespace Render {
struct RenderParams {
	const int w {};
	const int h {};
};
void Do(Context& ctx, const RenderParams p);
}
