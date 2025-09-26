#ifndef CPPXX_SQL_POSTGRES_H
#define CPPXX_SQL_POSTGRES_H

#include "../sql.h"
#include <cstring>
#include <netinet/in.h>
#include <postgresql/libpq-fe.h>
#include <chrono>
#include <string>
#include <optional>
#include <stdexcept>


namespace cppxx::sql::postgres::detail {
    struct Generic {
        const char *buffer;
        int length;
        int format; // 0=text, 1=binary
        Oid type;
        bool is_dynamic = false;
    };

    inline void free_generic(Generic &binder) {
        if (binder.is_dynamic)
            std::free(const_cast<char *>(binder.buffer));
    }

    template <typename T>
    struct Binder;

    template <typename T>
    struct Getter;

    template <typename T>
    Generic bind_one(const T &value) {
        return Binder<T>(value).bind();
    }

    template <typename T>
    T get_one(PGresult *res, int row, int col, bool is_binary = false) {
        return Getter<T>().get(res, row, col, is_binary);
    }

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
} // namespace cppxx::sql::postgres::detail


namespace cppxx::sql::postgres {
    class Connection;

    template <tuple_like Row>
    class Rows : public cppxx::sql::Rows<Row> {
        friend class Connection;

    protected:
        Rows(PGresult *res, int row_count, int pq_result_format = 0)
            : res(res),
              row_count(row_count),
              pq_result_format(pq_result_format) {}

    public:
        ~Rows() override { PQclear(res); }

        void next() override { ++current_row; }

        Row get() const override {
            if (current_row >= row_count)
                throw std::runtime_error("Failed to get row data: not a row");

            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::make_tuple(
                    detail::get_one<std::tuple_element_t<I, Row>>(res, current_row, int(I), pq_result_format)...);
            }(std::make_index_sequence<std::tuple_size_v<Row>>());
        }

        bool is_done() const override { return current_row >= row_count; }

    protected:
        PGresult *res;
        int row_count;
        int current_row{};
        int pq_result_format;
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
        Rows<Row> operator()(const Statement<Stmt, Params, Row> &statement, int pq_result_format = 0) {
            constexpr size_t N = std::tuple_size_v<Params>;

            const char *paramValues[N] = {};
            int paramLengths[N] = {};
            int paramFormats[N] = {}; // 0=text, 1=binary
            Oid paramTypes[N] = {};
            const int resultFormat = pq_result_format;

            std::vector<detail::Generic> binders = [&, this]<std::size_t... I>(std::index_sequence<I...>) {
                return std::vector<detail::Generic>{detail::bind_one(std::get<I>(statement.params))...};
            }(std::make_index_sequence<N>());

            for (size_t i = 0; i < N; i++) {
                paramValues[i] = binders[i].buffer;
                paramLengths[i] = binders[i].length;
                paramFormats[i] = binders[i].format;
                paramTypes[i] = binders[i].type;
            }

            std::string converted = detail::convert_placeholders(Stmt.value);
            PGresult *res =
                PQexecParams(db, converted.c_str(), N, paramTypes, paramValues, paramLengths, paramFormats, resultFormat);

            for (auto &binder : binders)
                detail::free_generic(binder);

            if (auto status = PQresultStatus(res); status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
                std::string what = PQerrorMessage(db);
                PQclear(res);
                std::string msg = "Failed to execute statement \"" + converted + "\": " + std::to_string(status) + ": " + what;
                throw std::runtime_error(msg);
            }

            int row_count = PQntuples(res);
            return {res, row_count, pq_result_format};
        }

        ~Connection() override { PQfinish(db); }

    protected:
        PGconn *db;
    };
} // namespace cppxx::sql::postgres

namespace cppxx::sql::postgres::detail {
    inline uint16_t to_network(uint16_t v) { return htons(v); }
    inline uint32_t to_network(uint32_t v) { return htonl(v); }
    inline uint64_t to_network(uint64_t v) {
#if defined(__APPLE__) || defined(__linux__)
        return htobe64(v); // available on BSD/Linux
#elif defined(_WIN32)
        return htonll(v); // available on MSVC
#else
        if constexpr (std::endian::native == std::endian::little) {

            return __builtin_bswap64(v);
        } else {
            return v;
        }
#endif
    }

    inline uint16_t from_network(uint16_t v) { return ntohs(v); }
    inline uint32_t from_network(uint32_t v) { return ntohl(v); }
    inline uint64_t from_network(uint64_t v) {
#if defined(__APPLE__) || defined(__linux__)
        return be64toh(v);
#elif defined(_WIN32)
        return ntohll(v);
#else
        if constexpr (std::endian::native == std::endian::little) {
            return __builtin_bswap64(v);
        } else {
            return v;
        }
#endif
    }

    inline int16_t to_network(int16_t v) {
        uint16_t u;
        std::memcpy(&u, &v, sizeof(u));
        u = htons(u);
        int16_t out;
        std::memcpy(&out, &u, sizeof(out));
        return out;
    }

    inline int16_t from_network(int16_t v) {
        uint16_t u;
        std::memcpy(&u, &v, sizeof(u));
        u = ntohl(u);
        int16_t out;
        std::memcpy(&out, &u, sizeof(out));
        return out;
    }

    inline int32_t to_network(int32_t v) {
        uint32_t u;
        std::memcpy(&u, &v, sizeof(u));
        u = htonl(u);
        int32_t out;
        std::memcpy(&out, &u, sizeof(out));
        return out;
    }

    inline int32_t from_network(int32_t v) {
        uint32_t u;
        std::memcpy(&u, &v, sizeof(u));
        u = ntohl(u);
        int32_t out;
        std::memcpy(&out, &u, sizeof(out));
        return out;
    }

    inline uint64_t bswap64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(x);
#elif defined(_MSC_VER)
        return _byteswap_uint64(x);
#else
        return ((x & 0x00000000000000FFULL) << 56) | ((x & 0x000000000000FF00ULL) << 40) | ((x & 0x0000000000FF0000ULL) << 24)
            | ((x & 0x00000000FF000000ULL) << 8) | ((x & 0x000000FF00000000ULL) >> 8) | ((x & 0x0000FF0000000000ULL) >> 24)
            | ((x & 0x00FF000000000000ULL) >> 40) | ((x & 0xFF00000000000000ULL) >> 56);
#endif
    }

    inline int64_t to_network(int64_t v) {
        uint64_t u;
        std::memcpy(&u, &v, sizeof(u));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        u = bswap64(u);
#endif
        int64_t out;
        std::memcpy(&out, &u, sizeof(out));
        return out;
    }

    inline int64_t from_network(int64_t v) {
        uint64_t u;
        std::memcpy(&u, &v, sizeof(u));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        u = bswap64(u);
#endif
        int64_t out;
        std::memcpy(&out, &u, sizeof(out));
        return out;
    }

    inline float to_network(float f) {
        static_assert(sizeof(float) == 4);
        uint32_t i;
        std::memcpy(&i, &f, sizeof(f));
        i = htonl(i);
        float out;

        std::memcpy(&out, &i, sizeof(out));
        return out;
    }

    inline double to_network(double d) {
        static_assert(sizeof(double) == 8);
        uint64_t i;
        std::memcpy(&i, &d, sizeof(d));
        i = to_network(i); // 64-bit version
        double out;
        std::memcpy(&out, &i, sizeof(out));
        return out;
    }

    // PostgreSQL OIDs for common types
    constexpr Oid PG_INT2OID = 21;    // smallint
    constexpr Oid PG_INT4OID = 23;    // integer
    constexpr Oid PG_INT8OID = 20;    // bigint
    constexpr Oid PG_FLOAT4OID = 700; // real
    constexpr Oid PG_FLOAT8OID = 701; // double precision

    template <typename T, typename = void>
    struct pg_type_oid;

    template <>
    struct pg_type_oid<int16_t> {
        static constexpr Oid value = PG_INT2OID;
    };

    template <>
    struct pg_type_oid<int32_t> {
        static constexpr Oid value = PG_INT4OID;
    };

    template <>
    struct pg_type_oid<int64_t> {
        static constexpr Oid value = PG_INT8OID;
    };

    // Specializations for floating-point
    template <>
    struct pg_type_oid<float> {
        static constexpr Oid value = PG_FLOAT4OID;
    };

    template <>
    struct pg_type_oid<double> {
        static constexpr Oid value = PG_FLOAT8OID;
    };

    template <std::signed_integral T>
    struct Binder<T> {
        T value;
        Generic bind() const {
            T *data = (T *)malloc(sizeof(T));
            *data = to_network(value);
            return {(const char *)data, sizeof(T), 1, pg_type_oid<T>::value, true};
        }
    };

    template <>
    struct Binder<bool> {
        bool value;
        Generic bind() const {
            char *data = (char *)malloc(1);
            *data = (char)value;
            return {data, 1, 1, 16, true};
        }
    };

    template <std::floating_point T>
    struct Binder<T> {
        T value;
        Generic bind() const {
            T *data = (T *)malloc(sizeof(T));
            *data = to_network(value);
            return {(const char *)data, sizeof(T), 1, pg_type_oid<T>::value, true};
        }
    };

    template <>
    struct Binder<std::string> {
        const std::string &value;
        Generic bind() const { return {value.c_str(), (int)value.size(), 0, 0, false}; }
    };

    template <>
    struct Binder<std::vector<uint8_t>> {
        const std::vector<uint8_t> &value;
        Generic bind() const { return {(const char *)value.data(), (int)value.size(), 1, 17, false}; }
    };

    template <>
    struct Binder<std::chrono::system_clock::time_point> {
        using T = std::chrono::system_clock::time_point;
        const T &value;
        Generic bind() const {
            std::time_t t = std::chrono::system_clock::to_time_t(value);
            char *data = (char *)malloc(64);
            std::strftime(data, 64, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
            return {data, 64, 0, 0, true};
        }
    };

    template <typename T>
    struct Binder<std::optional<T>> {
        const std::optional<T> &value;
        Generic bind() const {
            if (!value.has_value())
                return {nullptr, 0, 0, 0, false};
            return Binder<T>(*value).bind();
        }
    };

    template <std::signed_integral T>
    struct Getter<T> {
        T get(PGresult *res, int row, int col, bool is_binary) const {
            if (auto ptr = PQgetvalue(res, row, col); is_binary) {
                return from_network(*reinterpret_cast<T *>(ptr));
            } else {
                return (T)std::stoi(ptr);
            }
        }
    };

    template <>
    struct Getter<bool> {
        bool get(PGresult *res, int row, int col, bool is_binary) const {
            if (auto ptr = PQgetvalue(res, row, col); is_binary) {
                return *ptr;
            } else {
                std::string v = ptr;
                return (v == "t" || v == "true" || v == "1");
            }
        }
    };

    template <std::floating_point T>
    struct Getter<T> {
        T get(PGresult *res, int row, int col, bool is_binary) const {
            if (auto ptr = PQgetvalue(res, row, col); is_binary) {
                return from_network(*reinterpret_cast<T *>(ptr));
            } else {
                return (T)std::stod(ptr);
            }
        }
    };

    template <>
    struct Getter<std::string> {
        std::string get(PGresult *res, int row, int col, bool) const { return PQgetvalue(res, row, col); }
    };

    template <>
    struct Getter<std::vector<uint8_t>> {
        std::vector<uint8_t> get(PGresult *res, int row, int col, bool is_binary) const {
            if (auto ptr = PQgetvalue(res, row, col); is_binary) {
                auto end = ptr + PQgetlength(res, row, col);
                return {ptr, end};
            } else {
                throw std::runtime_error("TODO: text format for bytea is not supported yet");
            }
        }
    };

    template <>
    struct Getter<std::chrono::system_clock::time_point> {
        using T = std::chrono::system_clock::time_point;
        T get(PGresult *res, int row, int col, bool is_binary) const {
            if (auto ptr = PQgetvalue(res, row, col); is_binary) {
                throw std::runtime_error("TODO: binary format for time point is not supported yet");
            } else {
                std::tm tm{};
                strptime(ptr, "%Y-%m-%d %H:%M:%S", &tm);

                auto t = std::mktime(&tm);
                return std::chrono::system_clock::from_time_t(t);
            }
        }
    };

    template <typename T>
    struct Getter<std::optional<T>> {
        std::optional<T> get(PGresult *res, int row, int col, bool is_binary) const {
            if (PQgetisnull(res, row, col))
                return std::nullopt;
            return Getter<T>().get(res, row, col, is_binary);
        }
    };
} // namespace cppxx::sql::postgres::detail
#endif
