#ifndef CPPXX_SQL_SQLITE3_H
#define CPPXX_SQL_SQLITE3_H

#include "../sql.h"
#include <sqlite3.h>
#include <string>
#include <optional>
#include <stdexcept>


namespace cppxx::sql::sqlite3::detail {
    inline int bind_one(sqlite3_stmt *stmt, int index, int value) { return sqlite3_bind_int(stmt, index, value); }

    inline int bind_one(sqlite3_stmt *stmt, int index, sqlite3_int64 value) { return sqlite3_bind_int64(stmt, index, value); }

    inline int bind_one(sqlite3_stmt *stmt, int index, double value) { return sqlite3_bind_double(stmt, index, value); }

    inline int bind_one(sqlite3_stmt *stmt, int index, const std::string &value) {
        return sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    inline int bind_one(sqlite3_stmt *stmt, int index, const char *value) {
        return sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
    }

    inline int bind_one(sqlite3_stmt *stmt, int index, std::nullptr_t) { return sqlite3_bind_null(stmt, index); }

    template <typename T>
    int bind_one(sqlite3_stmt *stmt, int index, const std::optional<T> &value) {
        return value.has_value() ? bind_one(stmt, index, *value) : bind_one(stmt, index, nullptr);
    }

    template <typename T>
    std::optional<T> get_one(sqlite3_stmt *stmt, int index);

    template <>
    inline std::optional<int> get_one(sqlite3_stmt *stmt, int index) {
        if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
            return std::nullopt;
        return sqlite3_column_int(stmt, index);
    }

    template <>
    inline std::optional<int64_t> get_one(sqlite3_stmt *stmt, int index) {
        if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
            return std::nullopt;
        return sqlite3_column_int64(stmt, index);
    }

    template <>
    inline std::optional<float> get_one(sqlite3_stmt *stmt, int index) {
        if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
            return std::nullopt;
        return sqlite3_column_double(stmt, index);
    }

    template <>
    inline std::optional<double> get_one(sqlite3_stmt *stmt, int index) {
        if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
            return std::nullopt;
        return sqlite3_column_double(stmt, index);
    }

    template <>
    inline std::optional<std::string> get_one(sqlite3_stmt *stmt, int index) {
        if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
            return std::nullopt;
        return (const char *)sqlite3_column_text(stmt, index);
    }
} // namespace cppxx::sql::sqlite3::detail


namespace cppxx::sql::sqlite3 {
    template <tuple_like Row>
    class Rows {
    public:
        Rows(struct sqlite3 *db, sqlite3_stmt *stmt)
            : db(db),
              stmt(stmt) {
            next();
        }

        virtual ~Rows() { sqlite3_finalize(stmt); }

        void next() {
            ret = sqlite3_step(stmt);
            if (ret != SQLITE_DONE and ret != SQLITE_ROW) {
                std::string what = sqlite3_errmsg(db);
                std::string msg = "Failed to step: " + std::to_string(ret) + ": " + what;
                throw std::runtime_error(msg);
            }
        }

        auto get() const {
            if (ret != SQLITE_ROW) {
                std::string msg = "Failed to get row data: not a row";
                throw std::runtime_error(msg);
            }

            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::make_tuple(detail::get_one<std::tuple_element_t<I, Row>>(stmt, int(I))...);
            }(std::make_index_sequence<std::tuple_size_v<Row>>());
        }

        bool is_done() const { return ret == SQLITE_DONE; }

    protected:
        struct sqlite3 *db;
        sqlite3_stmt *stmt;
        int ret;
    };

    class Connection : public cppxx::sql::Connection {
    public:
        Connection(const std::string &filename) {
            int ret = sqlite3_open(filename.c_str(), &db);
            if (ret != SQLITE_OK) {
                std::string what = sqlite3_errmsg(db);
                sqlite3_close(db);
                std::string msg = "Cannot open \"" + filename + "\": " + what;
                throw std::runtime_error(msg);
            }
        }

        template <literal Stmt, tuple_like Params, tuple_like Row>
        Rows<Row> execute(const Statement<Stmt, Params, Row> &statement) {
            int ret = sqlite3_prepare_v2(db, Stmt.value, -1, &stmt, nullptr);
            if (ret != SQLITE_OK) {
                std::string what = sqlite3_errmsg(db);
                std::string msg = "Failed to prepare statement \"" + std::string(Stmt.value) + "\": " + what;
                throw std::runtime_error(msg);
            }

            [this, &statement]<std::size_t... I>(std::index_sequence<I...>) {
                (detail::bind_one(stmt, I + 1, std::get<I>(statement.params)), ...);
            }(std::make_index_sequence<std::tuple_size_v<Params>>());

            return {db, stmt};
        }

        ~Connection() override { sqlite3_close(db); }

    protected:
        struct sqlite3 *db;
        struct sqlite3_stmt *stmt;
    };
} // namespace cppxx::sql::sqlite3

#endif
