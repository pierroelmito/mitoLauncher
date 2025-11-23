
#include "launcher.hpp"

#include <chrono>

#include <boost/algorithm/string/replace.hpp>

#include "dbpp.hpp"
#include "sql.hpp"

namespace Update {

void SetLastPlayed(Context& ctx, Content& c)
{
	const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	Query(ctx.db, Sql::UpdateLastPlayed, [&](sqlite3_stmt* stmt) {
		sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(now));
		sqlite3_bind_int(stmt, 2, c.id);
	});
	c.lastPlayed = now;
}

void SetFav(Context& ctx, Content& c, bool fav)
{
	Query(ctx.db, Sql::UpdateFavorite, [&](sqlite3_stmt* stmt) {
		sqlite3_bind_int(stmt, 1, fav ? 1 : 0);
		sqlite3_bind_int(stmt, 2, c.id);
	});
	c.favorite = fav;
}

void MoveSelection(SelectVector& select, int direction, Mode mode)
{
	const auto sz = select.values.size();
	if (sz == 0)
		return;
	auto& s = select.vs.selected;
	if (direction > 0) {
		if (mode == Mode::Clamp)
			s = std::min(s + direction, sz - 1);
		else
			s = (s + direction) % sz;
	} else if (direction < 0) {
		if (mode == Mode::Clamp)
			s = (s + direction < sz) ? s + direction : 0;
		else
			s = (s + direction + sz) % sz;
	}
}

void KeyMove(Context& ctx, SelectVector& select)
{
	if (IsKeyPressed(ctx.mapping.down))
		MoveSelection(select, 1, Mode::Wrap);
	if (IsKeyPressed(ctx.mapping.up))
		MoveSelection(select, -1, Mode::Wrap);
	if (IsKeyPressed(ctx.mapping.pageDown))
		MoveSelection(select, 10, Mode::Clamp);
	if (IsKeyPressed(ctx.mapping.pageUp))
		MoveSelection(select, -10, Mode::Clamp);
}

SelectVector& PushView(Context& ctx, State st, const std::string& title, size_t sz, ItemType it)
{
	auto& nview = ctx.views.emplace_back();
	nview.state = st;
	nview.title = title;
	nview.values.clear();
	nview.values.reserve(ctx.content.size());
	for (size_t i = 0; i < sz; ++i) {
		nview.values.push_back({ it, i });
	}
	return nview;
}

bool Do_Platforms(Context& ctx, const UpdateParams)
{
	auto fillContent = [&](const std::string& pf, const char* query) {
		using R = DataReader<int, std::string, std::string, std::string, std::optional<std::string>, bool, std::optional<time_t>>;
		ctx.content = R::FetchStructs<Content>(ctx.db, query, [](const auto& row) {
			const auto& [id, name, path, platform, image, favorite, lp] = row;
			return Content { id, name, path, platform, image, favorite, lp };
		});
		if (!ctx.content.empty())
			PushView(ctx, State::Content, pf, ctx.content.size(), ItemType::Content).vs = ctx.lastSelected[pf];
	};
	if (IsKeyPressed(ctx.mapping.activate)) {
		auto& view = ctx.views.back();
		auto& row = view.values[view.vs.selected];
		if (row.it == ItemType::Platform) {
			const auto& [platform, count] = view.current(ctx.platforms);
			const char* query = TextFormat(Sql::SelectContent, platform.c_str());
			fillContent(platform, query);
		} else if (row.it == ItemType::View) {
			const auto& [name, count] = view.current(ctx.customViews);
			if (name == Str::Favorites) {
				fillContent("Favorites", Sql::SelectFavorites);
			} else if (name == Str::History) {
				fillContent("History", Sql::SelectHistory);
			} else if (name == Str::Themes) {
				PushView(ctx, State::Themes, "Themes", ctx.themeList.size(), ItemType::Theme).vs = {};
			}
		}
		return true;
	}
	return true;
}

bool Do_Content(Context& ctx, const UpdateParams)
{
	if (IsKeyPressed(ctx.mapping.activate)) {
		auto& view = ctx.views.back();
		auto& item = view.current(ctx.content);
		const auto cmd = [&]() -> std::string {
			auto it = ctx.platformInfo.find(item.platform);
			if (it != ctx.platformInfo.end() && !it->second.commandLine.empty())
				return boost::replace_all_copy(it->second.commandLine, "%P", item.path);
			return TextFormat("xdg-open \"%s\"", item.path.c_str());
		}();
		TraceLog(LOG_INFO, "Execute command: %s", cmd.c_str());
		view.values[view.vs.selected].dirty();
		SetLastPlayed(ctx, item);
		std::system(cmd.c_str());
		RefreshMain(ctx);
	} else if (IsKeyPressed(ctx.mapping.toggle)) {
		auto& view = ctx.views.back();
		auto& item = view.current(ctx.content);
		view.values[view.vs.selected].dirty();
		SetFav(ctx, item, !item.favorite);
		RefreshMain(ctx);
	}
	return true;
}

bool Do_Themes(Context& ctx, const UpdateParams)
{
	if (IsKeyPressed(ctx.mapping.activate)) {
		auto& view = ctx.views.back();
		const auto& [th] = view.current(ctx.themeList);
		ctx.theme = ctx.themes[th];
		return true;
	}
	return true;
}

bool Do(Context& ctx, const UpdateParams p)
{
	if (WindowShouldClose())
		return false;

	if (IsKeyPressed(ctx.mapping.back)) {
		auto& view = ctx.views.back();
		ctx.lastSelected[view.title] = view.vs;
		ctx.views.resize(ctx.views.size() - 1);
		return !ctx.views.empty();
	}

	// check font update
	if (ctx.currentFont != ctx.theme.fi) {
		if (!ctx.currentFont.fontPath.empty()) {
			UnloadFont(ctx.fnt);
		}
		if (ctx.theme.fi.fontPath.empty()) {
			ctx.fnt = GetFontDefault();
		} else {
			ctx.fnt = LoadFontEx(ctx.theme.fi.fontPath.c_str(), ctx.theme.fi.textSize, nullptr, 0);
		}
		ctx.currentFont = ctx.theme.fi;
		for (auto& v : ctx.views) {
			for (auto& val : v.values) {
				val.textWidth = 0;
			}
		}
	}

	{
		auto& view = ctx.views.back();
		KeyMove(ctx, view);
	}

	if (IsKeyPressed(KEY_N)) {
		ctx.nm = !ctx.nm;
	}

	const auto state = ctx.views.back().state;
	if (state == State::Platforms) {
		return Do_Platforms(ctx, p);
	} else if (state == State::Content) {
		return Do_Content(ctx, p);
	} else if (state == State::Themes) {
		return Do_Themes(ctx, p);
	}

	return true;
}

} // namespace Update
