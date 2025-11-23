
#include "launcher.hpp"

#include <filesystem>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "dbpp.hpp"
#include "sql.hpp"
#include "tclpp.hpp"

void AddPlatform(Context& ctx, sqlite3_stmt* stmt, const std::string& name, const std::string& label)
{
	sqlite3_reset(stmt);
	sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, label.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		TraceLog(LOG_ERROR, "Failed to execute statement: %s", sqlite3_errmsg(ctx.db));
	}
}

void AddPlatforms(Context& ctx)
{
	sqlite3_stmt* stmt;
	const char* sql = "INSERT OR IGNORE INTO platforms (name, label) VALUES (?, ?);";
	if (sqlite3_prepare_v2(ctx.db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		TraceLog(LOG_ERROR, "Failed to prepare statement: %s", sqlite3_errmsg(ctx.db));
		return;
	}
	for (const auto& [id, pi] : ctx.platformInfo) {
		AddPlatform(ctx, stmt, id, pi.label);
	}
	sqlite3_finalize(stmt);
}

void InitTcl(Context& ctx)
{
	ctx.interp = Tcl_CreateInterp();

#define OPT_VALUE(X) X = TclTo<decltype(X)::value_type>(tcl, argv[++i])

	constexpr auto platform = [](ClientData cd, Tcl_Interp* tcl, int argc, Tcl_Obj* const* argv) -> int {
		std::optional<std::string> name;
		std::optional<std::string> command;
		std::optional<std::string> label;
		for (int i = 1; i < argc; ++i) {
			const auto arg = TclTo<std::string>(tcl, argv[i]);
			if (i < argc - 1) {
				if (arg == "-name") {
					OPT_VALUE(name);
				} else if (arg == "-command") {
					OPT_VALUE(command);
				} else if (arg == "-label") {
					OPT_VALUE(label);
				}
			}
		}
		if (!name)
			return TCL_ERROR;
		Context& ctx = *((Context*)cd);
		TraceLog(LOG_INFO, "Add platform %s [%s] [%s]",
			name->c_str(),
			label.value_or("?").c_str(),
			command.value_or("?").c_str());
		const auto sz = ctx.platformInfo.size();
		auto& pi = ctx.platformInfo[*name];
		if (sz != ctx.platformInfo.size())
			pi.label = *name;
		if (command)
			pi.commandLine = *command;
		if (label)
			pi.label = *label;
		return TCL_OK;
	};

	constexpr auto directory = [](ClientData cd, Tcl_Interp* tcl, int argc, Tcl_Obj* const* argv) -> int {
		std::optional<std::string> platform;
		std::optional<std::string> dir;
		std::optional<std::string> image;
		std::optional<std::vector<std::string>> optExt;
		std::optional<std::vector<int>> depth;
		for (int i = 1; i < argc; ++i) {
			const auto arg = TclTo<std::string>(tcl, argv[i]);
			if (i < argc - 1) {
				if (arg == "-platform") {
					OPT_VALUE(platform);
				} else if (arg == "-dir") {
					OPT_VALUE(dir);
				} else if (arg == "-ext") {
					OPT_VALUE(optExt);
				} else if (arg == "-depth") {
					OPT_VALUE(depth);
				} else if (arg == "-image") {
					OPT_VALUE(image);
				}
			}
		}
		if (!platform)
			return TCL_ERROR;
		Context& ctx = *((Context*)cd);
		auto it = ctx.platformInfo.find(*platform);
		if (it == ctx.platformInfo.end())
			return TCL_ERROR;
		PlatformInfo& pi = it->second;
		if (dir) {
			auto ext = optExt.value_or({});
			for (auto& e : ext)
				e = "." + e;
			pi.dirs.push_back({ *dir, image, { ext.begin(), ext.end() } });
			const std::string exts = boost::algorithm::join(ext, "|");
			TraceLog(LOG_INFO, "Add directory [%s] [%s] [%s] [%s]",
				platform->c_str(),
				image.value_or({}).c_str(),
				dir->c_str(),
				exts.c_str());
		}
		return TCL_OK;
	};

	constexpr auto theme = [](ClientData cd, Tcl_Interp* tcl, int argc, Tcl_Obj* const* argv) -> int {
		std::optional<std::string> name;
		std::optional<std::string> bg;
		std::optional<std::string> fg;
		std::optional<std::string> border;
		std::optional<std::string> mid;
		std::optional<int> rh;
		std::optional<int> th;
		std::optional<std::string> font;
		for (int i = 1; i < argc; ++i) {
			const auto arg = TclTo<std::string>(tcl, argv[i]);
			if (i < argc - 1) {
				if (arg == "-name") {
					OPT_VALUE(name);
				} else if (arg == "-fg") {
					OPT_VALUE(fg);
				} else if (arg == "-bg") {
					OPT_VALUE(bg);
				} else if (arg == "-border") {
					OPT_VALUE(border);
				} else if (arg == "-mid") {
					OPT_VALUE(mid);
				} else if (arg == "-rh") {
					OPT_VALUE(rh);
				} else if (arg == "-th") {
					OPT_VALUE(th);
				} else if (arg == "-fn") {
					OPT_VALUE(font);
				}
			}
		}
		if (!name)
			return TCL_ERROR;
		Context& ctx = *((Context*)cd);
		Theme& theme = ctx.themes[*name];
		const auto strToColor = [](const std::string& hex) -> std::optional<Color> {
			if (hex.size() == 7 && hex[0] == '#') {
				const auto number = (uint32_t)std::strtol(hex.c_str() + 1, NULL, 16);
				return GetColor((number << 8) | 0xff);
			}
			return std::nullopt;
		};
		if (bg)
			theme.colors[ColorType::Bg] = strToColor(*bg).value_or(theme.colors[ColorType::Bg]);
		if (fg)
			theme.colors[ColorType::Fg] = strToColor(*fg).value_or(theme.colors[ColorType::Fg]);
		if (border)
			theme.colors[ColorType::Border] = strToColor(*border).value_or(theme.colors[ColorType::Border]);
		if (mid)
			theme.colors[ColorType::Mid] = strToColor(*mid).value_or(theme.colors[ColorType::Mid]);
		if (rh)
			theme.rowSize = *rh;
		if (th)
			theme.fi.textSize = *th;
		if (font)
			theme.fi.fontPath = *font;
		ctx.theme = theme;
		return TCL_OK;
	};

#undef OPT_VALUE

	Tcl_CreateObjCommand(ctx.interp, "platform", platform, &ctx, nullptr);
	Tcl_CreateObjCommand(ctx.interp, "directory", directory, &ctx, nullptr);
	Tcl_CreateObjCommand(ctx.interp, "theme", theme, &ctx, nullptr);

	auto r = Tcl_EvalFile(ctx.interp, APP_NAME ".tcl");
	if (r != TCL_OK) {
		TraceLog(LOG_ERROR, "Tcl_EvalFile error: %s", Tcl_GetStringResult(ctx.interp));
	}
}

void InitDB(Context& ctx)
{
	auto& db = ctx.db;

	if (sqlite3_open_v2(APP_NAME ".db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
		TraceLog(LOG_ERROR, "Cannot open database: %s", sqlite3_errmsg(db));
		return;
	}

	char* errMsg = nullptr;
	if (sqlite3_exec(db, Sql::InitDB, nullptr, nullptr, &errMsg) != SQLITE_OK) {
		TraceLog(LOG_ERROR, "SQL error: %s", errMsg);
		sqlite3_free(errMsg);
	}
}

std::string applyPattern(const std::string& p, const std::filesystem::path& path)
{
	std::string r = p;
	r = boost::replace_all_copy(r, "%d", path.parent_path().string());
	r = boost::replace_all_copy(r, "%f", path.stem().string());
	return r;
}

void ScanContentRec(Context& ctx, sqlite3_stmt* stmt, const std::string& platform, const std::filesystem::path& path, int minDepth, int maxDepth, const DirectoryInfo& di, int depth = 0)
{
	for (const auto& p : std::filesystem::directory_iterator(path)) {
		if (p.is_directory()) {
			if (depth < maxDepth)
				ScanContentRec(ctx, stmt, platform, p.path(), minDepth, maxDepth, di, depth + 1);
		} else if (p.is_regular_file()) {
			const bool depthOk = depth >= minDepth && depth <= maxDepth;
			if (!depthOk)
				continue;
			const std::string vpath = p.path().string();
			const std::string e = p.path().extension().string();
			if (!di.ext.empty() && !di.ext.contains(e))
				continue;
			const std::string vname = p.path().stem().string();
			TraceLog(LOG_INFO, "Accept - p: %s / vname: %s / vpath: %s", p.path().string().c_str(), vname.c_str(), vpath.c_str());
			std::optional<std::string> img {};
			if (di.imagePattern) {
				img = applyPattern(*di.imagePattern, p);
				TraceLog(LOG_INFO, "Image path: %s -> %s", di.imagePattern->c_str(), img->c_str());
			}
			sqlite3_reset(stmt);
			sqlite3_bind_text(stmt, 1, vname.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(stmt, 2, vpath.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(stmt, 3, platform.c_str(), -1, SQLITE_STATIC);
			if (img)
				sqlite3_bind_text(stmt, 4, img->c_str(), -1, SQLITE_STATIC);
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				TraceLog(LOG_ERROR, "Failed to execute statement: %s", sqlite3_errmsg(ctx.db));
			}
		}
	}
}

void ScanContent(Context& ctx, const std::string& platform, const std::filesystem::path& path, const DirectoryInfo& di)
{
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(ctx.db, Sql::InsertContent, -1, &stmt, nullptr) != SQLITE_OK) {
		TraceLog(LOG_ERROR, "Failed to prepare statement: %s", sqlite3_errmsg(ctx.db));
		return;
	}
	ScanContentRec(ctx, stmt, platform, path, 0, 0, di);
	sqlite3_finalize(stmt);
}

void RefreshContent(Context& ctx)
{
	for (const auto& [id, pi] : ctx.platformInfo) {
		TraceLog(LOG_INFO, "Scan %s", id.c_str());
		for (const auto& di : pi.dirs) {
			TraceLog(LOG_INFO, "    Dir %s", di.dir.c_str());
			ScanContent(ctx, id, di.dir, di);
		}
	}
	RefreshMain(ctx);
}

void RefreshMain(Context& ctx)
{
	{
		using R = DataReader<std::string, int>;
		ctx.platforms = R::FetchTuples(ctx.db, Sql::SelectPlatforms);
	}

	{
		const auto& [countFav] = DataReader<int>::FetchOne(ctx.db, Sql::CountFav);
		const auto& [countHist] = DataReader<int>::FetchOne(ctx.db, Sql::CountHist);
		ctx.customViews = {
			{ Str::History, countHist },
			{ Str::Favorites, countFav },
			{ Str::Themes, ctx.themes.size() },
		};
	}

	{
		auto& view = ctx.views.front();
		view.values.clear();
		view.values.reserve(ctx.platforms.size() + ctx.customViews.size());
		for (size_t i = 0; i < ctx.platforms.size(); ++i) {
			view.values.push_back({ ItemType::Platform, i });
		}
		for (size_t i = 0; i < ctx.customViews.size(); ++i) {
			view.values.push_back({ ItemType::View, i });
		}
	}
}

bool Init(Context& ctx, std::span<char*>)
{
#ifndef DEBUG
	SetTraceLogLevel(LOG_WARNING);
#endif

	InitWindow(1280, 720, "main");
	SetWindowState(FLAG_VSYNC_HINT);
	SetExitKey(KEY_NULL);
	SetTargetFPS(60);
	DisableCursor();

	InitDB(ctx);
	InitTcl(ctx);
	AddPlatforms(ctx);

	{
		ctx.themeList.clear();
		for (const auto& [id, th] : ctx.themes) {
			ctx.themeList.push_back(id);
		}
	}

#if 0
	{
		const std::vector<KeyboardKey> keys = {
			KEY_APOSTROPHE,
			KEY_COMMA,
			KEY_MINUS,
			KEY_PERIOD,
			KEY_SLASH,
			KEY_ZERO,
			KEY_ONE,
			KEY_TWO,
			KEY_THREE,
			KEY_FOUR,
			KEY_FIVE,
			KEY_SIX,
			KEY_SEVEN,
			KEY_EIGHT,
			KEY_NINE,
			KEY_SEMICOLON,
			KEY_EQUAL,
			KEY_A,
			KEY_B,
			KEY_C,
			KEY_D,
			KEY_E,
			KEY_F,
			KEY_G,
			KEY_H,
			KEY_I,
			KEY_J,
			KEY_K,
			KEY_L,
			KEY_M,
			KEY_N,
			KEY_O,
			KEY_P,
			KEY_Q,
			KEY_R,
			KEY_S,
			KEY_T,
			KEY_U,
			KEY_V,
			KEY_W,
			KEY_X,
			KEY_Y,
			KEY_Z,
			KEY_LEFT_BRACKET,
			KEY_BACKSLASH,
			KEY_RIGHT_BRACKET,
			KEY_GRAVE,
			KEY_SPACE,
			KEY_ESCAPE,
			KEY_ENTER,
			KEY_TAB,
			KEY_BACKSPACE,
			KEY_INSERT,
			KEY_DELETE,
			KEY_RIGHT,
			KEY_LEFT,
			KEY_DOWN,
			KEY_UP,
			KEY_PAGE_UP,
			KEY_PAGE_DOWN,
			KEY_HOME,
			KEY_END,
			KEY_CAPS_LOCK,
			KEY_SCROLL_LOCK,
			KEY_NUM_LOCK,
			KEY_PRINT_SCREEN,
			KEY_PAUSE,
			KEY_F1,
			KEY_F2,
			KEY_F3,
			KEY_F4,
			KEY_F5,
			KEY_F6,
			KEY_F7,
			KEY_F8,
			KEY_F9,
			KEY_F10,
			KEY_F11,
			KEY_F12,
			KEY_LEFT_SHIFT,
			KEY_LEFT_CONTROL,
			KEY_LEFT_ALT,
			KEY_LEFT_SUPER,
			KEY_RIGHT_SHIFT,
			KEY_RIGHT_CONTROL,
			KEY_RIGHT_ALT,
			KEY_RIGHT_SUPER,
			KEY_KB_MENU,
			KEY_KP_0,
			KEY_KP_1,
			KEY_KP_2,
			KEY_KP_3,
			KEY_KP_4,
			KEY_KP_5,
			KEY_KP_6,
			KEY_KP_7,
			KEY_KP_8,
			KEY_KP_9,
			KEY_KP_DECIMAL,
			KEY_KP_DIVIDE,
			KEY_KP_MULTIPLY,
			KEY_KP_SUBTRACT,
			KEY_KP_ADD,
			KEY_KP_ENTER,
			KEY_KP_EQUAL,
			KEY_BACK,
			KEY_MENU,
			KEY_VOLUME_UP,
			KEY_VOLUME_DOWN,
		};
		for (auto key : keys) {
			const char* name = GetKeyName(key);
			if (name != nullptr && name[0] != '\0') {
				TraceLog(LOG_INFO, "Key %d -> '%s'", key, name);
			} else {
				TraceLog(LOG_INFO, "Key %d -> ?", key);
			}
		}
	}
#endif

	ctx.views.emplace_back(); // create main view
	RefreshContent(ctx);

	return true;
}
