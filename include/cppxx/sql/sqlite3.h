#ifndef CPPXX_SQL_SQLITE3_H
#define CPPXX_SQL_SQLITE3_H

#include "../sql.h"
#include <sqlite3.h>
#include <string>
#include <optional>
#include <stdexcept>


namespace cppxx::sql::sqlite3::detail {
    template <typename T>
    struct Binder;

    template <typename T>
    struct Getter;

    template <typename T>
    void bind_one(sqlite3_stmt *stmt, int index, const T &value) {
        Binder<T> binder(value);
        binder.bind(stmt, index);
    };

    template <typename T>
    T get_one(sqlite3_stmt *stmt, int index) {
        Getter<T> getter;
        return getter.get(stmt, index);
    };
} // namespace cppxx::sql::sqlite3::detail

namespace cppxx::sql::sqlite3 {
    class Connection;

    template <tuple_like Row>
    class Rows : cppxx::sql::Rows<Row> {
        friend class Connection;

    protected:
        Rows(struct sqlite3 *db, sqlite3_stmt *stmt)
            : db(db),
              stmt(stmt) {
            next();
        }

    public:
        virtual ~Rows() { sqlite3_finalize(stmt); }

        void next() override {
            ret = sqlite3_step(stmt);
            if (ret != SQLITE_DONE and ret != SQLITE_ROW) {
                std::string what = sqlite3_errmsg(db);
                std::string msg = "Failed to step: " + std::to_string(ret) + ": " + what;
                throw std::runtime_error(msg);
            }
        }

        Row get() const override {
            if (ret != SQLITE_ROW)
                throw std::runtime_error("Failed to get row data: not a row");

            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::make_tuple(detail::get_one<std::tuple_element_t<I, Row>>(stmt, int(I))...);
            }(std::make_index_sequence<std::tuple_size_v<Row>>());
        }

        bool is_done() const override { return ret == SQLITE_DONE; }

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
        Rows<Row> operator()(const Statement<Stmt, Params, Row> &statement) {
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

namespace cppxx::sql::sqlite3::detail {
    template <>
    struct Binder<int> {
        int value;
        void bind(sqlite3_stmt *stmt, int index) const { sqlite3_bind_int(stmt, index, value); }
    };

    template <>
    struct Binder<sqlite3_int64> {
        sqlite3_int64 value;
        void bind(sqlite3_stmt *stmt, int index) const { sqlite3_bind_int64(stmt, index, value); }
    };

    template <>
    struct Binder<double> {
        double value;
        void bind(sqlite3_stmt *stmt, int index) const { sqlite3_bind_double(stmt, index, value); }
    };

    template <>
    struct Binder<std::string> {
        const std::string &value;
        void bind(sqlite3_stmt *stmt, int index) const {
            sqlite3_bind_text(stmt, index, value.c_str(), (int)value.size(), SQLITE_TRANSIENT);
        }
    };

    template <typename T>
    struct Binder<std::optional<T>> {
        const std::optional<T> &value;
        void bind(sqlite3_stmt *stmt, int index) const {
            if (!value.has_value())
                sqlite3_bind_null(stmt, index);
            else
                Binder<T>(*value).bind(stmt, index);
        }
    };

    template <>
    struct Getter<int> {
        int get(sqlite3_stmt *stmt, int index) const { return sqlite3_column_int(stmt, index); }
    };

    template <>
    struct Getter<sqlite3_int64> {
        sqlite3_int64 get(sqlite3_stmt *stmt, int index) const { return sqlite3_column_int64(stmt, index); }
    };

    template <>
    struct Getter<double> {
        double get(sqlite3_stmt *stmt, int index) const { return sqlite3_column_double(stmt, index); }
    };

    template <>
    struct Getter<std::string> {
        std::string get(sqlite3_stmt *stmt, int index) const { return (const char *)sqlite3_column_text(stmt, index); }
    };

    template <typename T>
    struct Getter<std::optional<T>> {
        std::optional<T> bind(sqlite3_stmt *stmt, int index) const {
            if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
                return std::nullopt;
            else
                return Getter<T>().get(stmt, index);
        }
    };
} // namespace cppxx::sql::sqlite3::detail

#endif
