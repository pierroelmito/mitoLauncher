
#include "launcher.hpp"

#include <chrono>
#include <format>

namespace Render {

inline Vector2 TextPos(int x, int y)
{
	return { float(x), float(y) };
}

template <typename F>
std::tuple<int, int> FsList(Context& ctx, const RenderParams p, SelectVector& select, const F& func)
{
	const int rowHeight = ctx.theme.rowSize;
	const int rowCount = p.h / rowHeight - 2;
	const int headerHeight = (p.h - rowCount * rowHeight) / 2;
	const int footerHeight = p.h - rowCount * rowHeight - headerHeight;

	if (select.vs.selected < select.vs.viewStart) {
		select.vs.viewStart = select.vs.selected;
	} else if (select.vs.selected >= select.vs.viewStart + rowCount) {
		select.vs.viewStart = select.vs.selected - rowCount + 1;
	}

	const auto border = ctx.theme.get(ctx.nm, ColorType::Border);
	DrawRectangle(0, 0, p.w, headerHeight, border);
	DrawRectangle(0, p.h - footerHeight, p.w, footerHeight, border);

	int y = headerHeight;
	for (size_t i = select.vs.viewStart; i < select.values.size() && i < select.vs.viewStart + rowCount; ++i) {
		const bool selected = i == select.vs.selected;
		func(selected, y, rowHeight, i, select.values[i]);
		y += rowHeight;
	}

	return { headerHeight, footerHeight };
}

void Header(Context& ctx, const RenderParams, int hh, const std::string& title)
{
	const int th = ctx.theme.fi.textSize;
	const int dy = (hh - th) / 2;
	const auto fg = ctx.theme.get(ctx.nm, ColorType::Fg);
	DrawTextEx(ctx.fnt, TextFormat("%s", title.c_str()), TextPos(10, dy), th, 2, fg);
}

void Footer(Context& ctx, const RenderParams p, int fh)
{
	const int th = ctx.theme.fi.textSize;
	const int dy = (fh - th) / 2;
	auto tSysTime = std::chrono::system_clock::now();
	auto hour = std::format("{:%H:%M:%S}", floor<std::chrono::seconds>(tSysTime));
	const int w = MeasureTextEx(ctx.fnt, hour.c_str(), th, 2).x;
	const auto fg = ctx.theme.get(ctx.nm, ColorType::Fg);
	DrawTextEx(ctx.fnt, hour.c_str(), TextPos(p.w - w - 10, p.h - fh + dy), th, 2, fg);
}

void Row(Context& ctx, const RenderParams, int y, int dy, ViewItem& item, bool selected)
{
	const int th = ctx.theme.fi.textSize;
	const int rs = ctx.theme.rowSize;
	if (item.textWidth == 0)
		item.textWidth = MeasureTextEx(ctx.fnt, item.label.c_str(), th, 2).x;
	const int w = item.textWidth;
	const int o = 6;
	const int sx = rs + 10;
	const auto bg = ctx.theme.get(ctx.nm, ColorType::Bg);
	const auto fg = ctx.theme.get(ctx.nm, ColorType::Fg);
	if (selected)
		DrawRectangle(sx - o, y + 1, w + 2 * o, rs - 2, fg);
	DrawTextEx(ctx.fnt, item.label.c_str(), TextPos(sx, y + dy), th, 2, selected ? bg : fg);
}

void Do_Platforms(Context& ctx, const RenderParams p)
{
	auto& view = ctx.views.back();
	const auto [hh, fh] = FsList(ctx, p, view, [&](bool selected, int y, int h, int, ViewItem& row) {
		if (row.label.empty()) {
			if (row.it == ItemType::Platform) {
				const auto& item = ctx.platforms[row.index];
				const auto& [platform, count] = item;
				const auto it = ctx.platformInfo.find(platform);
				const auto& pl = it != ctx.platformInfo.end() ? it->second.label : platform;
				const char* text = TextFormat("%s (%d)", pl.c_str(), count);
				row.label = text;
			} else if (row.it == ItemType::View) {
				const auto& item = ctx.customViews[row.index];
				const auto& [label, count] = item;
				const char* text = TextFormat("< %s > (%d)", label.c_str(), count);
				row.label = text;
			}
		}
		const int th = ctx.theme.fi.textSize;
		const int dy = (h + 2 - th) / 2;
		Row(ctx, p, y, dy, row, selected);
	});
	const std::string label = "Platforms";
	const char* header = TextFormat("%s (%d / %d)", label.c_str(), view.vs.selected + 1, int(view.values.size()));
	Header(ctx, p, hh, header);
	Footer(ctx, p, fh);
}

void Do_Content(Context& ctx, const RenderParams p)
{
	auto& view = ctx.views.back();
	const auto [hh, fh] = FsList(ctx, p, view, [&](bool, int y, int h, int index, const auto& row) {
		const auto& item = ctx.content[row.index];
		const int rs = ctx.theme.rowSize;
		const int th = ctx.theme.fi.textSize;
		const int dy = (h + 2 - th) / 2;
		const auto& name = item.name;
		if (index == 0 || name[0] != ctx.content[view.values[index - 1].index].name[0]) {
			const auto bg = ctx.theme.get(ctx.nm, ColorType::Bg);
			const auto mid = ctx.theme.get(ctx.nm, ColorType::Mid);
			DrawRectangle(1, y + 1, p.w - 2, 1, mid);
			DrawRectangle(1, y + 1, rs, rs - 2, mid);
			DrawTextEx(ctx.fnt, TextFormat("%c", name[0]), TextPos(10, y + dy), th, 2, bg);
		}
	});
	{
		auto& current = view.current(ctx.content);
		if (current.image) {
			if (current.tex.width == 0) {
				current.tex = LoadTexture(current.image->c_str());
			}
			if (current.tex.width != 0) {
				const float o = 100;
				const float tw = current.tex.width;
				const float th = current.tex.height;
				const float dh = p.h - 2 * o;
				const float dw = dh * tw / th;
				const Rectangle dest = { p.w - dw - 60, o, dw, dh };
				DrawTexturePro(current.tex, { 0, 0, tw, th }, dest, {}, 0, { 255, 255, 255, 255 });
				DrawRectangleLinesEx(dest, 1, ctx.theme.get(ctx.nm, ColorType::Fg));
			}
		}
	}
	FsList(ctx, p, view, [&](bool selected, int y, int h, int /*index*/, ViewItem& row) {
		if (row.label.empty()) {
			const auto& item = ctx.content[row.index];
			const auto& name = item.name;
			const bool hasLp = item.lastPlayed.has_value();
			const char* fmtFav = item.favorite ? "* " : "";
			const char* fmtLp0 = hasLp ? "[" : "";
			const char* fmtLp1 = hasLp ? "]" : "";
			const char* text = TextFormat("%s%s%s%s", fmtFav, fmtLp0, name.c_str(), fmtLp1);
			row.label = text;
		}
		const int th = ctx.theme.fi.textSize;
		const int dy = (h + 2 - th) / 2;
		Row(ctx, p, y, dy, row, selected);
	});
	const auto it = ctx.platformInfo.find(view.title);
	const auto& label = it != ctx.platformInfo.end() ? it->second.label : view.title;
	const char* header = TextFormat("%s (%d / %d)", label.c_str(), view.vs.selected + 1, int(view.values.size()));
	Header(ctx, p, hh, header);
	Footer(ctx, p, fh);
}

void Do_Themes(Context& ctx, const RenderParams p)
{
	auto& view = ctx.views.back();
	const auto [hh, fh] = FsList(ctx, p, view, [&](bool selected, int y, int h, int, ViewItem& row) {
		if (row.label.empty()) {
			if (row.it == ItemType::Theme) {
				const auto& item = ctx.themeList[row.index];
				const auto& [theme] = item;
				row.label = theme;
			}
		}
		const int th = ctx.theme.fi.textSize;
		const int dy = (h + 2 - th) / 2;
		Row(ctx, p, y, dy, row, selected);
	});
	const std::string label = "Themes";
	const char* header = TextFormat("%s (%d / %d)", label.c_str(), view.vs.selected + 1, int(view.values.size()));
	Header(ctx, p, hh, header);
	Footer(ctx, p, fh);
}

void Do(Context& ctx, const RenderParams p)
{
	ctx.frame++;
	BeginDrawing();
	const auto bg = ctx.theme.get(ctx.nm, ColorType::Bg);
	ClearBackground(bg);
	const auto state = ctx.views.back().state;
	if (state == State::Platforms) {
		Do_Platforms(ctx, p);
	} else if (state == State::Content) {
		Do_Content(ctx, p);
	} else if (state == State::Themes) {
		Do_Themes(ctx, p);
	}
	EndDrawing();
}

} // namespace Render
