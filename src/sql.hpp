#pragma once

struct Sql {
	static constexpr const char* InitDB = R"(
		CREATE TABLE IF NOT EXISTS platforms (
			id INTEGER PRIMARY KEY,
			name TEXT NOT NULL,
			label TEXT NOT NULL,
			UNIQUE(name)
		);
		
		CREATE TABLE IF NOT EXISTS content (
			id INTEGER PRIMARY KEY,
			name TEXT NOT NULL,
			path TEXT NOT NULL,
			platform TEXT NOT NULL,
			image TEXT,
			favorite INTEGER DEFAULT 0,
			last_played DATETIME DEFAULT null,
			UNIQUE(path, platform)
		);
	)";
	static constexpr const char* CountFav = R"(
		SELECT count(*) from content WHERE favorite = 1
	)";
	static constexpr const char* CountHist = R"(
		SELECT count(*) from content WHERE last_played is not null ORDER BY last_played
	)";
	static constexpr const char* InsertContent = R"(
		INSERT OR IGNORE INTO content (name, path, platform, image) VALUES (?, ?, ?, ?);
	)";
	static constexpr const char* SelectPlatforms = R"(
		SELECT platform, count(platform) FROM content, platforms WHERE content.platform = platforms.name GROUP BY platform ORDER BY label;
	)";
	static constexpr const char* SelectContent = R"(
		SELECT id, name, path, platform, image, favorite, last_played FROM content WHERE platform = '%s' ORDER BY name;
	)";
	static constexpr const char* SelectFavorites = R"(
		SELECT id, name, path, platform, image, favorite, last_played FROM content WHERE favorite = 1 ORDER BY name;
	)";
	static constexpr const char* SelectHistory = R"(
		SELECT id, name, path, platform, image, favorite, last_played FROM content WHERE last_played is not null ORDER BY last_played;)";
	static constexpr const char* UpdateLastPlayed = R"(
		UPDATE content SET last_played = ? WHERE id = ?;
	)";
	static constexpr const char* UpdateFavorite = R"(
		UPDATE content SET favorite = ? WHERE id = ?;
	)";
};
