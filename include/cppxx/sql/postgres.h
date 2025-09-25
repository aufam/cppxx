#ifndef CPPXX_SQL_POSTGRES_H
#define CPPXX_SQL_POSTGRES_H

#include "../sql.h"
#include <postgresql/libpq-fe.h>
#include <chrono>
#include <string>
#include <optional>
#include <variant>
#include <stdexcept>


namespace cppxx::sql::postgres::detail {
    using Bind = std::variant<std::string, const std::string *>;

    struct BoundVisitor {
        const char *operator()(const std::string &str) const { return str.data(); }
        const char *operator()(const std::string *str) const { return str->data(); }
    };

    std::string convert_placeholders(const std::string &query) {
        std::string result;
        result.reserve(query.size() + 16); // small optimization

        int counter = 1;
        for (char c : query) {
            if (c == '?') {
                result += "$" + std::to_string(counter++);
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    inline void bind_one(int value, Bind &bound) { bound = std::to_string(value); }
    inline void bind_one(double value, Bind &bound) { bound = std::to_string(value); }
    inline void bind_one(bool value, Bind &bound) { bound = value ? "true" : "false"; }
    inline void bind_one(const std::string &value, Bind &bound) { bound = &value; }
    inline void bind_one(std::nullptr_t, Bind &bound) { bound = nullptr; }

    inline void bind_one(std::chrono::system_clock::time_point value, Bind &bound) {
        std::time_t t = std::chrono::system_clock::to_time_t(value);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        bound = buf;
    }

    template <typename T>
    inline void bind_one(const std::optional<T> &value, Bind &bound) {
        value.has_value() ? bind_one(*value, bound) : bind_one(nullptr, bound);
    }

    template <typename T>
    std::optional<T> get_one(PGresult *res, int row, int col);

    template <>
    inline std::optional<int> get_one<int>(PGresult *res, int row, int col) {
        if (PQgetisnull(res, row, col))
            return std::nullopt;
        return std::stoi(PQgetvalue(res, row, col));
    }

    template <>
    inline std::optional<double> get_one<double>(PGresult *res, int row, int col) {
        if (PQgetisnull(res, row, col))
            return std::nullopt;
        return std::stod(PQgetvalue(res, row, col));
    }

    template <>
    inline std::optional<std::string> get_one<std::string>(PGresult *res, int row, int col) {
        if (PQgetisnull(res, row, col))
            return std::nullopt;
        return std::string(PQgetvalue(res, row, col));
    }

    template <>
    inline std::optional<bool> get_one<bool>(PGresult *res, int row, int col) {
        if (PQgetisnull(res, row, col))
            return std::nullopt;
        std::string v = PQgetvalue(res, row, col);
        return (v == "t" || v == "true" || v == "1");
    }

    template <>
    inline std::optional<std::chrono::system_clock::time_point> get_one(PGresult *res, int row, int col) {
        if (PQgetisnull(res, row, col))
            return std::nullopt;
        std::string s = PQgetvalue(res, row, col);

        std::tm tm{};
        strptime(s.c_str(), "%Y-%m-%d %H:%M:%S", &tm);

        auto t = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(t);
    }
} // namespace cppxx::sql::postgres::detail


namespace cppxx::sql::postgres {
    template <tuple_like Row>
    class Rows {
    public:
        Rows(PGresult *res, int row_count)
            : res(res),
              row_count(row_count) {}

        virtual ~Rows() { PQclear(res); }

        void next() { ++current_row; }

        auto get() const {
            if (current_row >= row_count)
                throw std::runtime_error("Failed to get row data: not a row");

            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::make_tuple(detail::get_one<std::tuple_element_t<I, Row>>(res, current_row, int(I))...);
            }(std::make_index_sequence<std::tuple_size_v<Row>>());
        }

        bool is_done() const { return current_row >= row_count; }


    protected:
        PGresult *res;
        int row_count;
        int current_row{};
    };

    class Connection : public cppxx::sql::Connection {
    public:
        Connection(const std::string &spec) {
            db = PQconnectdb(spec.c_str());
            if (PQstatus(db) != CONNECTION_OK) {
                std::string what = PQerrorMessage(db);
                PQfinish(db);

                std::string msg = "Cannot connect with spec=\"" + spec + "\": " + what;
                throw std::runtime_error(msg);
            }
        }

        template <literal Stmt, tuple_like Params, tuple_like Row>
        Rows<Row> execute(const Statement<Stmt, Params, Row> &statement) {
            constexpr size_t N = std::tuple_size_v<Params>;

            Bind binders[N];
            const char *paramValues[N] = {};
            int paramLengths[N] = {};
            int paramFormats[N] = {}; // 0=text, 1=binary
            Oid paramTypes[N] = {};
            const int resultFormat = 0;

            [&, this]<std::size_t... I>(std::index_sequence<I...>) {
                ((detail::bind_one(std::get<I>(statement.params), binders[I])), ...);
            }(std::make_index_sequence<N>());

            for (size_t i = 0; i < N; i++)
                paramValues[i] = std::visit(detail::BoundVisitor{}, binders[i]);

            std::string converted = detail::convert_placeholders(Stmt.value);
            PGresult *res =
                PQexecParams(db, converted.c_str(), N, paramTypes, paramValues, paramLengths, paramFormats, resultFormat);

            if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
                std::string what = PQerrorMessage(db);
                PQclear(res);
                std::string msg = "Failed to execute statement \"" + converted + "\": " + what;
                throw std::runtime_error(msg);
            }

            int row_count = PQntuples(res);
            return {res, row_count};
        }

        ~Connection() override { PQfinish(db); }

    protected:
        PGconn *db;
    };
} // namespace cppxx::sql::postgres

#endif
