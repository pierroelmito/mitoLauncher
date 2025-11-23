#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <raylib.h>
#include <sqlite3.h>

template <typename T>
struct ValueFetcher {
	static T FetchValue(sqlite3_stmt* stmt, int index)
	{
		if constexpr (std::is_same_v<T, int>) {
			return sqlite3_column_int(stmt, index);
		} else if constexpr (std::is_same_v<T, std::string>) {
			const unsigned char* text = sqlite3_column_text(stmt, index);
			return text ? reinterpret_cast<const char*>(text) : "";
		} else if constexpr (std::is_same_v<T, bool>) {
			return sqlite3_column_int(stmt, index) != 0;
		} else if constexpr (std::is_same_v<T, time_t>) {
			return sqlite3_column_int64(stmt, index) != 0;
		} else {
			static_assert(false, "unhandled type");
		}
		return {};
	}
};

template <typename T>
struct ValueFetcher<std::optional<T>> {
	static std::optional<T> FetchValue(sqlite3_stmt* stmt, int index)
	{
		if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
			return std::nullopt;
		return ValueFetcher<T>::FetchValue(stmt, index);
	}
};

template <typename... T>
struct DataReader {
	template <int... Is>
	static void FetchRowImpl(sqlite3_stmt* stmt, std::tuple<T...>& row, std::integer_sequence<int, Is...>)
	{
		([&]() {
			using CurrentType = typename std::tuple_element<Is, std::tuple<T...>>::type;
			std::get<Is>(row) = ValueFetcher<CurrentType>::FetchValue(stmt, Is);
		}(),
			...);
	}
	template <typename F>
	static std::vector<std::tuple<T...>> VisitAll(sqlite3* db, const char* sql, const F& func)
	{
		std::vector<std::tuple<T...>> results;
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			TraceLog(LOG_ERROR, "Failed to prepare statement: %s", sqlite3_errmsg(db));
			return results;
		}
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			std::tuple<T...> row {};
			FetchRowImpl(stmt, row, std::make_integer_sequence<int, sizeof...(T)>());
			func(row);
		}
		sqlite3_finalize(stmt);
		return results;
	}
	static std::vector<std::tuple<T...>> FetchTuples(sqlite3* db, const char* sql)
	{
		std::vector<std::tuple<T...>> results;
		VisitAll(db, sql, [&](const std::tuple<T...>& row) {
			results.push_back(row);
		});
		return results;
	}
	static std::tuple<T...> FetchOne(sqlite3* db, const char* sql)
	{
		std::tuple<T...> result;
		VisitAll(db, sql, [&](const std::tuple<T...>& row) {
			result = row;
		});
		return result;
	}
	template <typename S>
	static std::vector<S> FetchStructs(sqlite3* db, const char* sql, S (*t)(const std::tuple<T...>&))
	{
		std::vector<S> results;
		VisitAll(db, sql, [&](const std::tuple<T...>& row) {
			results.push_back(t(row));
		});
		return results;
	}
};

template <typename F>
void Query(sqlite3* db, const char* sql, const F& func)
{
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		TraceLog(LOG_ERROR, "Failed to prepare statement: %s", sqlite3_errmsg(db));
		return;
	}
	func(stmt);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		TraceLog(LOG_ERROR, "Failed to execute statement: %s", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}
