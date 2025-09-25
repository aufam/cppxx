#ifndef CPPXX_SQL_MYSQL_H
#define CPPXX_SQL_MYSQL_H

#include "../sql.h"
#include <cstring>
#include <mysql/mysql.h>
#include <chrono>
#include <string>
#include <optional>
#include <variant>
#include <stdexcept>


namespace cppxx::sql::mysql::detail {
    void free_binds(std::vector<MYSQL_BIND> &binds, bool is_result) {
        for (auto &bind : binds) {
            if (is_result) {
                free(bind.buffer);
            }

            delete bind.length;
            delete bind.is_null;
            delete bind.error;
        }
    }

    inline void bind_one(MYSQL_BIND &bind, const int &value) {
        memset(&bind, 0, sizeof(bind));
        bind.buffer_type = MYSQL_TYPE_LONG;
        bind.buffer = const_cast<int *>(&value);
        bind.is_null = nullptr;
    }

    inline void bind_one(MYSQL_BIND &bind, const std::string &value) {
        memset(&bind, 0, sizeof(bind));
        bind.buffer_type = MYSQL_TYPE_STRING;
        bind.buffer = const_cast<char *>(value.data());
        bind.buffer_length = value.size();
        bind.length = new unsigned long(value.size()); // MySQL requires length pointer
        bind.is_null = nullptr;
    }

    template <typename T>
    std::optional<T> get_one(const MYSQL_BIND &bind);

    template <>
    inline std::optional<int> get_one(const MYSQL_BIND &bind) {
        if (*bind.is_null)
            return std::nullopt;
        return *static_cast<int *>(bind.buffer);
    }

    template <>
    inline std::optional<std::string> get_one(const MYSQL_BIND &bind) {
        if (*bind.is_null)
            return std::nullopt;
        return std::string(static_cast<char *>(bind.buffer), *bind.length);
    }

    template <typename T>
    void prepare_result_bind(MYSQL_BIND &bind);

    template <>
    void prepare_result_bind<int>(MYSQL_BIND &bind) {
        bind.buffer_type = MYSQL_TYPE_LONG;
        bind.buffer = malloc(sizeof(int));
        bind.is_null = new bool(false);
        bind.length = nullptr;
    }

    template <>
    void prepare_result_bind<std::string>(MYSQL_BIND &bind) {
        bind.buffer_type = MYSQL_TYPE_STRING;
        bind.buffer = malloc(1024); // choose max expected size
        bind.buffer_length = 1024;
        bind.is_null = new bool(false);
        bind.length = new unsigned long(0);
    }
} // namespace cppxx::sql::mysql::detail

namespace cppxx::sql::mysql {
    template <tuple_like Row>
    class Rows {
    public:
        Rows(MYSQL_STMT *stmt)
            : stmt(stmt) {
            bind_result();
            next();
        }

        virtual ~Rows() {
            mysql_stmt_free_result(stmt);
            mysql_stmt_close(stmt);
            detail::free_binds(result_binds, true);
        }

        void next() {
            int ret = mysql_stmt_fetch(stmt);
            if (ret == 0) {
                done = false;
            } else if (ret == MYSQL_NO_DATA || result_binds.empty()) {
                done = true;
            } else {
                unsigned int err = mysql_stmt_errno(stmt);
                std::string what = mysql_stmt_error(stmt);
                std::string msg = "Failed to fetch: " + std::to_string(err) + ": " + std::to_string(ret) + ": " + what;
                throw std::runtime_error(msg);
            }
        }

        auto get() const {
            if (done)
                throw std::runtime_error("Not a row");
            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::make_tuple(detail::get_one<std::tuple_element_t<I, Row>>(result_binds[I])...);
            }(std::make_index_sequence<std::tuple_size_v<Row>>());
        }

        bool is_done() const { return done; }

    protected:
        void bind_result() {
            result_binds.resize(std::tuple_size_v<Row>);

            [this]<size_t... I>(std::index_sequence<I...>) {
                ((detail::prepare_result_bind<std::tuple_element_t<I, Row>>(result_binds[I])), ...);
            }(std::make_index_sequence<std::tuple_size_v<Row>>());

            if (!result_binds.empty() && mysql_stmt_bind_result(stmt, result_binds.data())) {
                unsigned int err = mysql_stmt_errno(stmt);
                std::string what = mysql_stmt_error(stmt);
                std::string msg = "Bind result failed: " + std::to_string(err) + ": " + what;
                throw std::runtime_error(msg);
            }
        }

        MYSQL_STMT *stmt;
        bool done = false;
        std::vector<MYSQL_BIND> result_binds;
    };

    class Connection : public cppxx::sql::Connection {
    public:
        Connection(const std::string &host,
                   const std::string &user,
                   const std::string &pass,
                   const std::string &db,
                   unsigned int port = 3306) {
            mysql = mysql_init(nullptr);
            if (!mysql) {
                throw std::runtime_error("mysql_init() failed");
            }

            if (!mysql_real_connect(mysql, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, nullptr, 0)) {
                std::string what = mysql_error(mysql);
                unsigned int err = mysql_errno(mysql);
                std::string msg = "Cannot connect: " + std::to_string(err) + ": " + what;
                mysql_close(mysql);
                throw std::runtime_error(msg);
            }
        }

        template <literal Stmt, tuple_like Params, tuple_like Row>
        Rows<Row> execute(const Statement<Stmt, Params, Row> &statement) {
            MYSQL_STMT *stmt = mysql_stmt_init(mysql);
            if (!stmt) {
                throw std::runtime_error("mysql_stmt_init() failed");
            }

            if (mysql_stmt_prepare(stmt, Stmt.value, strlen(Stmt.value))) {
                std::string what = mysql_error(mysql);
                unsigned int err = mysql_errno(mysql);
                std::string msg =
                    "Failed to prepare statement `" + std::string(Stmt.value) + "`: " + std::to_string(err) + ": " + what;
                mysql_close(mysql);
                throw std::runtime_error(msg);
            }

            std::vector<MYSQL_BIND> param_binds(std::tuple_size_v<Params>);
            [this, &statement, &param_binds]<std::size_t... I>(std::index_sequence<I...>) {
                (detail::bind_one(param_binds[I], std::get<I>(statement.params)), ...);
            }(std::make_index_sequence<std::tuple_size_v<Params>>());

            if (!param_binds.empty() && mysql_stmt_bind_param(stmt, param_binds.data())) {
                unsigned int err = mysql_stmt_errno(stmt);
                std::string what = mysql_stmt_error(stmt);
                std::string msg = "Bind param failed: " + std::to_string(err) + ": " + what;
                mysql_close(mysql);
                detail::free_binds(param_binds, false);
                throw std::runtime_error(msg);
            }

            if (mysql_stmt_execute(stmt)) {
                unsigned int err = mysql_stmt_errno(stmt);
                std::string what = mysql_stmt_error(stmt);
                std::string msg = "Execute failed: " + std::to_string(err) + ": " + what;
                mysql_close(mysql);
                detail::free_binds(param_binds, false);
                throw std::runtime_error(msg);
            }

            detail::free_binds(param_binds, false);
            return Rows<Row>(stmt);
        }

        ~Connection() override { mysql_close(mysql); }

    protected:
        MYSQL *mysql;
    };


} // namespace cppxx::sql::mysql

#endif
